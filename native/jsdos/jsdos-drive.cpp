//
// Created by caiii on 23.08.2023.
//

#include <cassert>
#include <jsdos-drive.h>
#include <sockdrive.h>

#include <cstdio>

constexpr uint32_t diskSize = 2097152;
constexpr uint32_t sectorSize = 512;

jsdos::SockDrive::SockDrive(const std::string& host, uint16_t port):
    imageDisk(nullptr, (std::string(host) + ":" + std::to_string(port)).c_str(), diskSize, true) {
    handle = sockdrive_open(host.c_str(), port);
    const uint64_t heads = (double) diskSize / sectorSize / 520 / 63 * 1024;
    this->Set_Geometry(heads, 520, 63, sectorSize);
}

jsdos::SockDrive::~SockDrive() {
    sockdrive_close(handle);
}

uint8_t jsdos::SockDrive::Read_AbsoluteSector(uint32_t sectnum, void* data) {
    int errcode = sockdrive_read(handle, sectnum, (uint8_t*) data);
    if (errcode) {
        std::cerr << "sockdrive_read error " << errcode << std::endl;
        abort();
    }
    return 0;
}

uint8_t jsdos::SockDrive::Write_AbsoluteSector(uint32_t sectnum, const void* data) {
    int errcode = sockdrive_write(handle, sectnum, (uint8_t*) data);
    if (errcode) {
        std::cerr << "sockdrive_write error " << errcode << std::endl;
        abort();
    }
    return 0;
}
