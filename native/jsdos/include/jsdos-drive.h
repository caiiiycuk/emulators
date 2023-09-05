//
// Created by caiii on 23.08.2023.
//

#ifndef JS_DOS_DRIVE_H
#define JS_DOS_DRIVE_H

#include "bios_disk.h"

namespace jsdos {
    class SockDrive: public imageDisk {
        size_t handle;
    public:
        SockDrive(const std::string& host, uint16_t port);

        virtual ~SockDrive();

        virtual uint8_t Read_AbsoluteSector(uint32_t sectnum, void* data) override;

        virtual uint8_t Write_AbsoluteSector(uint32_t sectnum, const void* data) override;
    };
}

#endif //JS_DOS_DRIVE_H
