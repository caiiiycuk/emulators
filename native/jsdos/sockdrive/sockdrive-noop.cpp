//
// Created by caiiiycuk on 19.03.25.
//

#include <protocol.h>

void server_sockdrive_open(uint32_t handle, const char* address) {
  client_sockdrive_opened(handle, 0, 0, 0, 0, 0, 0, 0, 0);
}
extern void server_sockdrive_ready(uint32_t handle) {}
void server_sockdrive_close(uint32_t handle) {}
void server_sockdrive_load_range(uint32_t handle, uint32_t range) {}
void server_sockdrive_write_sector(uint32_t handle, uint32_t sector, uint8_t* buffer) {}
