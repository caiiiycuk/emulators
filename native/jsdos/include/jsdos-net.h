//
// Created by caiiiycuk on 30.09.2025.
//

#ifndef JS_DOS_JSDOS_NET_H
#define JS_DOS_JSDOS_NET_H
#include "protocol.h"

namespace jsdos {
  typedef uint32_t PeerId;
  struct Peer {
    PeerId id;
    bool server;
  };
  extern PeerId myPeerId;

  struct WsBuffer {
    PeerId peerId;
    int len;
    void *data;
  };

  int wsSend(Peer to, const void *datap, int len);
  int wsRecv(Peer* from, void *datap, int maxlen);
  void wsClose(PeerId peerId);
}

#endif  // JS_DOS_JSDOS_NET_H
