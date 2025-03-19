//
// Created by caiii on 30.08.2023.
//

#ifndef SOCKDRIVE_H
#define SOCKDRIVE_H

#include <stdint.h>
#include <functional>

uint32_t sockdrive_open(const char* url);
uint8_t sockdrive_read(uint32_t handle, uint32_t sector, uint8_t * buffer);
uint8_t sockdrive_write(uint32_t handle, uint32_t sector, uint8_t* buffer);
uint32_t sockdrive_size(uint32_t handle);
uint32_t sockdrive_heads(uint32_t handle);
uint32_t sockdrive_sectors(uint32_t handle);
uint32_t sockdrive_cylinders(uint32_t handle);
uint32_t sockdrive_sector_size(uint32_t handle);
void sockdrive_close(uint32_t handle);

#endif //SOCKDRIVE_H
