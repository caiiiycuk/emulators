//
// Created by caiiiycuk on 30.09.2025.
//
#include "jsdos-net.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <list>

#ifdef EMSCRIPTEN
#include <emscripten.h>
#endif

using namespace jsdos;

namespace {
  std::list<WsBuffer> wsBuffers;
  std::list<WsBuffer> wsServerBuffers;
}

PeerId jsdos::myPeerId = 0;

#ifdef EMSCRIPTEN
extern "C" void EMSCRIPTEN_KEEPALIVE setMyPeerId(uint32_t peerId) {
  jsdos::myPeerId = peerId;
}
#endif

int jsdos::wsSend(Peer peer, const void *datap, int len) {
    if (peer.id == myPeerId && peer.server) {
        void* copy = malloc(len);
        memcpy(copy, datap, len);
        wsServerBuffers.push_back({
          peer.id,
          len,
          copy
        });
        return len;
    } else if (peer.id == myPeerId) {
      void* copy = malloc(len);
      memcpy(copy, datap, len);
      wsBuffers.push_back({
        peer.id,
        len,
        copy
      });
      return len;
    }
    return server_net_send(peer.id, datap, len);
}

namespace {
int _wsRecv(Peer *peer, void *datap, int maxlen, std::list<WsBuffer> &wsBuffers) {
  if (wsBuffers.empty()) {
    return  0;
  }

  auto buffer = wsBuffers.front();
  wsBuffers.pop_front();

  if (buffer.len > maxlen) {
    printf("wsBuffer is bigger than provided buffer\n");
    abort();
  }

  memcpy(datap, buffer.data, buffer.len);
  free(buffer.data);
  peer->id = buffer.peerId;
  return buffer.len;
}
}


int jsdos::wsRecv(Peer *peer, void *datap, int maxlen) {
  return _wsRecv(peer, datap, maxlen, peer->server ? wsServerBuffers : wsBuffers);
}

void jsdos::wsClose(PeerId peerId) {
    if (peerId == myPeerId) {
      /* was never connected */
      return;
    }
    server_net_disconnect(peerId);
}

extern bool isIpxServer;
void client_net_recv(PeerId peer_id, void *datap, int len) {
    (isIpxServer ? wsServerBuffers : wsBuffers).push_back({
      peer_id,
      len,
      datap
    });
}
