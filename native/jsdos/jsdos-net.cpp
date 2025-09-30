//
// Created by caiiiycuk on 30.09.2025.
//
#include "jsdos-net.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <list>

using namespace jsdos;

namespace {
  std::list<WsBuffer> wsBuffers;
}

PeerId jsdos::myPeerId = 0;

int jsdos::wsSend(PeerId peerId, const void *datap, int len) {
    if (peerId == myPeerId) {
        void* copy = malloc(len);
        memcpy(copy, datap, len);
        client_net_recv(peerId, copy, len);
        return len;
    }
    return server_net_send(peerId, datap, len);
}

int jsdos::wsRecv(PeerId *peerId, void *datap, int maxlen) {
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
    *peerId = buffer.peerId;
    return buffer.len;
}

void jsdos::wsClose(PeerId peerId) {
    server_net_disconnect(peerId);
}

void client_net_recv(PeerId peer_id, void *datap, int len) {
    wsBuffers.push_back({
      peer_id,
      len,
      datap
    });
}
