#include <sockdrive.h>

size_t sockdrive_open(const char* url,
                      const char* owner, const char* name, const char* token) {
    return 0;
}

uint8_t sockdrive_read(size_t handle, uint32_t sector, uint8_t * buffer) {
    return 5;
}

uint8_t sockdrive_write(size_t handle, uint32_t sector, uint8_t* buffer) {
    return 5;
}

uint32_t sockdrive_size(size_t handle) {
    return 0;
}
uint32_t sockdrive_heads(size_t handle) {
    return 0;
}
uint32_t sockdrive_sectors(size_t handle) {
    return 0;
}
uint32_t sockdrive_cylinders(size_t handle) {
    return 0;
}
uint32_t sockdrive_sector_size(size_t handle) {
    return 0;
}
void sockdrive_close(size_t handle) {
}