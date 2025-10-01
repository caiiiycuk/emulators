#if C_IPX

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "ipxserver.h"
#include "jsdos-net.h"
#include "timer.h"
#undef socket

typedef struct _UDPsocket *UDPsocket;
typedef struct {
  int channel;       /* The src/dst channel of the packet */
  Uint8 *data;       /* The packet data */
  int len;           /* The length of the packet data */
  int maxlen;        /* The size of the data buffer */
  int status;        /* packet status after sending */
  IPaddress address; /* The source/dest address of an incoming/outgoing packet */
} UDPpacket;

Bit8u inBuffer[IPXBUFFERSIZE];
packetBuffer connBuffer[SOCKETTABLESIZE];
PackedIP ipconnguest[SOCKETTABLESIZE];  // the MAC address associated with each connection
IPaddress ipconn[SOCKETTABLESIZE];      // Active TCP/IP connection
UDPsocket tcpconn[SOCKETTABLESIZE];     // Active TCP/IP connections
IPaddress ipxServerIp;                  // IPAddress for server's listening port

static void sendIPXPacket(uint8_t *buffer, int16_t bufSize) {
  uint16_t srcport, destport;
  uint32_t srchost, desthost;
  uint16_t i;
  Bits result;
  UDPpacket outPacket;
  outPacket.channel = -1;
  outPacket.data = buffer;
  outPacket.len = bufSize;
  outPacket.maxlen = bufSize;
  IPXHeader *tmpHeader;
  tmpHeader = (IPXHeader *)buffer;

  srchost = tmpHeader->src.addr.byIP.host;
  desthost = tmpHeader->dest.addr.byIP.host;

  srcport = tmpHeader->src.addr.byIP.port;
  destport = tmpHeader->dest.addr.byIP.port;

  if (desthost == 0xffffffff) {
    // Broadcast
    for (i = 0; i < SOCKETTABLESIZE; i++) {
      if (connBuffer[i].connected && ((ipconnguest[i].host != srchost) || (ipconnguest[i].port != srcport))) {
        outPacket.address = ipconn[i];
        result = jsdos::wsSend({outPacket.address.host, false}, outPacket.data, outPacket.len);
        if (result == 0) {
          printf("IPXSERVER: can't send broadcast to %d\n", outPacket.address.host);
          continue;
        }
        // LOG_MSG("IPXSERVER: Packet of %d bytes sent from %d.%d.%d.%d to %d.%d.%d.%d (BROADCAST) (%x CRC)", bufSize,
        // CONVIP(srchost), CONVIP(ipconn[i].host), packetCRC(&buffer[30], bufSize-30));
      }
    }
  } else {
    // Specific address
    for (i = 0; i < SOCKETTABLESIZE; i++) {
      if ((connBuffer[i].connected) && (ipconnguest[i].host == desthost) && (ipconnguest[i].port == destport)) {
        outPacket.address = ipconn[i];
        result = jsdos::wsSend({outPacket.address.host, false}, outPacket.data, outPacket.len);
        if (result == 0) {
          printf("IPXSERVER: can't send to %d\n", outPacket.address.host);
          continue;
        }
        // LOG_MSG("IPXSERVER: Packet sent from %d.%d.%d.%d to %d.%d.%d.%d", CONVIP(srchost), CONVIP(desthost));
      }
    }
  }
}

static void ackClient(IPaddress clientAddr, bool extAck, PackedIP *guestmac);

extern bool ConnectToServer(char const *strAddr);
jsdos::Peer inPeer = {
    0,
    true,
};
extern void IPX_ServerLoop() {
  if (jsdos::myPeerId == 0) {
    printf("ERR! Network (myPeerId) is not set\n");
    abort();
  }

  ipxServerIp.host = jsdos::myPeerId;
  ipxServerIp.port = 213;

  UDPpacket inPacket;
  IPaddress tmpAddr;

  // char regString[] = "IPX Register\0";

  uint16_t i;
  uint32_t host;
  int result;

  inPacket.channel = -1;
  inPacket.data = &inBuffer[0];
  inPacket.maxlen = IPXBUFFERSIZE;

  result = jsdos::wsRecv(&inPeer, inPacket.data, inPacket.maxlen);
  inPacket.address.host = inPeer.id;
  inPacket.address.port = 213;
  inPacket.len = result;
  if (result != 0) {
    // Check to see if incoming packet is a registration packet
    // For this, I just spoofed the echo protocol packet designation 0x02
    IPXHeader *tmpHeader;
    tmpHeader = (IPXHeader *)&inBuffer[0];

    // Check to see if echo packet
    if (SDLNet_Read16(tmpHeader->dest.socket) == 0x2) {
      // Null destination node means it's a server registration packet
      if (tmpHeader->dest.addr.byIP.host == 0x0) {
        UnpackIP(tmpHeader->src.addr.byIP, &tmpAddr);
        for (i = 0; i < SOCKETTABLESIZE; i++) {
          if (!connBuffer[i].connected) {
            bool extAck = false;

            // Use preferred host IP rather than the reported source IP
            // It may be better to use the reported source
            ipconn[i] = inPacket.address;

            // Other DOSBox forks may expect the MAC address to match the IP host + port combined. Default behavior.
            ipconnguest[i].host = inPacket.address.host;
            ipconnguest[i].port = inPacket.address.port;

            // Allow client to register their own MAC address. Guest MAC address sits just after header at offset 30.
            if (tmpHeader->transControl == (unsigned char)'M' && inPacket.len >= (30 + 6)) {
              printf(
                  "IPXSERVER: Allowing client to register their own MAC address (DOSBox-X extension) "
                  "%02x:%02x:%02x:%02x:%02x:%02x\n",
                  inBuffer[30], inBuffer[31], inBuffer[32], inBuffer[33], inBuffer[34], inBuffer[35]);
              memcpy(&ipconnguest[i], &inBuffer[30], 6);
              extAck = true;
            }

            connBuffer[i].connected = true;
            host = ipconn[i].host;
            printf("IPXSERVER: Connect from %d.%d.%d.%d\n", CONVIP(host));
            ackClient(inPacket.address, extAck, &ipconnguest[i]);
            return;
          } else {
            if ((ipconnguest[i].host == tmpAddr.host) && (ipconnguest[i].port == tmpAddr.port)) {
              printf("IPXSERVER: Reconnect from %d.%d.%d.%d\n", CONVIP(tmpAddr.host));
              // Update anonymous port number if changed
              ipconn[i].port = inPacket.address.port;
              ackClient(inPacket.address, false, &ipconnguest[i]);
              return;
            }
          }
        }
      }
    }

    // IPX packet is complete.  Now interpret IPX header and send to respective IP address
    sendIPXPacket(inPacket.data, inPacket.len);
  }
}

bool IPX_StartServer(Bit16u portnum) {
  TIMER_AddTickHandler(&IPX_ServerLoop);
  return true;
}

static void ackClient(IPaddress clientAddr, bool extAck, PackedIP *guestmac) {
  IPXHeader regHeader;
  UDPpacket regPacket;

  SDLNet_Write16(0xffff, regHeader.checkSum);
  SDLNet_Write16(sizeof(regHeader), regHeader.length);

  SDLNet_Write32(0, regHeader.dest.network);
  PackIP(clientAddr, &regHeader.dest.addr.byIP);
  SDLNet_Write16(0x2, regHeader.dest.socket);

  SDLNet_Write32(1, regHeader.src.network);
  PackIP(ipxServerIp, &regHeader.src.addr.byIP);
  SDLNet_Write16(0x2, regHeader.src.socket);
  regHeader.transControl = 0;

  /* This is a way for the client to know whether the extension worked or not */
  if (extAck && guestmac != NULL) {
    memcpy(&regHeader.dest.addr.byNode, guestmac, 6);
    regHeader.transControl = (unsigned char)'M';
  }

  regPacket.data = (Uint8 *)&regHeader;
  regPacket.len = sizeof(regHeader);
  regPacket.maxlen = sizeof(regHeader);
  regPacket.address = clientAddr;
  // Send registration string to client.  If client doesn't get this, client will not be registered
  jsdos::wsSend({inPeer.id, false}, regPacket.data, regPacket.maxlen);
}

void IPX_StopServer() { TIMER_DelTickHandler(&IPX_ServerLoop); }

bool IPX_isConnectedToServer(Bits tableNum, IPaddress **ptrAddr) {
  if (tableNum >= SOCKETTABLESIZE) return false;
  *ptrAddr = &ipconn[tableNum];
  return connBuffer[tableNum].connected;
}

#endif
