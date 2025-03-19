//
// Created by caiiiycuk on 23.08.2023.
//

#include <jsdos-drive.h>

#include <cassert>
#include <cstdio>
#include <sockdrive.h>

jsdos::SockDrive* jsdos::SockDrive::create(const std::string& url) {
  auto handle = sockdrive_open(url.c_str());
  if (handle) {
    return new jsdos::SockDrive(handle, url);
  }

  return nullptr;
}

jsdos::SockDrive::SockDrive(size_t handle, const std::string& url)
    : imageDisk::imageDisk(nullptr, url.c_str(), sockdrive_size(handle), true), handle(handle) {
  this->Set_Geometry(sockdrive_heads(handle), sockdrive_cylinders(handle), sockdrive_sectors(handle),
                     sockdrive_sector_size(handle));
}

jsdos::SockDrive::~SockDrive() { sockdrive_close(handle); }

uint8_t jsdos::SockDrive::Read_AbsoluteSector(uint32_t sectnum, void* data) {
  return sockdrive_read(handle, sectnum, (uint8_t*)data);
}

uint8_t jsdos::SockDrive::Write_AbsoluteSector(uint32_t sectnum, const void* data) {
  int errcode = sockdrive_write(handle, sectnum, (uint8_t*)data);
  if (errcode) {
    std::cerr << "sockdrive_write error " << errcode << std::endl;
    abort();
  }
  return errcode;
}
