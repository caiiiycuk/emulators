//
// Created by caiii on 23.08.2023.
//

#ifndef JS_DOS_DRIVE_H
#define JS_DOS_DRIVE_H

#include "bios_disk.h"
#include <stdint.h>

namespace jsdos {
    class SockDrive: public imageDisk {
        size_t handle;
        SockDrive(size_t handle, const std::string& url, const std::string& owner, const std::string& name);
    public:
        static bool asyncRead;

        virtual ~SockDrive();
        virtual uint8_t Read_AbsoluteSector(uint32_t sectnum, void* data) override;
        virtual uint8_t Write_AbsoluteSector(uint32_t sectnum, const void* data) override;

        static SockDrive* create(const std::string& url, const std::string& owner, const std::string& name);
    };
}

#endif //JS_DOS_DRIVE_H
