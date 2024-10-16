//
// Created by caiiiycuk on 23.08.2023.
//

#include <cassert>
#include <jsdos-drive.h>
#include <sockdrive.h>
#include <cstdio>

void sockdrive_delay();
bool jsdos::SockDrive::asyncRead = false;

jsdos::SockDrive* jsdos::SockDrive::create(const std::string& url, const std::string& owner, const std::string& name) {
    auto handle = sockdrive_open(url.c_str(), owner.c_str(), name.c_str(), "");
    if (handle) {
       return new jsdos::SockDrive(handle, url, owner, name); 
    }

    return nullptr;
}

jsdos::SockDrive::SockDrive(size_t handle, const std::string& url, const std::string& owner, const std::string& name):
    imageDisk::imageDisk(nullptr, (url + "{" + owner + "/" + name + "}").c_str(), sockdrive_size(handle), true), handle(handle) {
    this->Set_Geometry(sockdrive_heads(handle), sockdrive_cylinders(handle), sockdrive_sectors(handle), sockdrive_sector_size(handle));
}

jsdos::SockDrive::~SockDrive() {
    sockdrive_close(handle);
}

uint8_t jsdos::SockDrive::Read_AbsoluteSector(uint32_t sectnum, void* data) {
    bool async = jsdos::SockDrive::asyncRead;
    uint8_t errcode = sockdrive_read(handle, sectnum, (uint8_t*) data, async);
    if (errcode && (!async || errcode != 255)) {
        return errcode;
    }

    while (async && errcode == 255) {
        sockdrive_delay();
        errcode = sockdrive_read_async_code(handle, sectnum, (uint8_t*) data);
    }

    return errcode;
}

uint8_t jsdos::SockDrive::Write_AbsoluteSector(uint32_t sectnum, const void* data) {
    int errcode = sockdrive_write(handle, sectnum, (uint8_t*) data);
    if (errcode) {
        std::cerr << "sockdrive_write error " << errcode << std::endl;
        abort();
    }
    return errcode;
}
