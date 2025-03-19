//
// Created by caiii on 08.02.2024.
//

#ifndef SOCKDRIVE_BLOCKCACHE_H
#define SOCKDRIVE_BLOCKCACHE_H

#include <unistd.h>

#include <cstring>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

class RangeCache {
    std::mutex mutex;
    const uint32_t sectorSize;
    const uint32_t aheadRange;
    std::unordered_map<uint32_t, std::unique_ptr<std::vector<uint8_t>>> cache;

public:
    RangeCache(uint32_t sectorSize, uint32_t aheadRange):
        sectorSize(sectorSize), aheadRange(aheadRange) {
    }

    uint8_t *read(uint32_t sector) {
        std::lock_guard<std::mutex> lock(mutex);
        const auto range = getRange(sector);
        auto it = cache.find(range);
        if (it != cache.end()) {
            // Sector 8277 belongs to range 16
            return it->second->data() +
                (sector * sectorSize - range * aheadRange);
        }
        return nullptr;
    }

    void create(uint32_t range, const uint8_t *buffer) {
        std::lock_guard<std::mutex> lock(mutex);
        auto it = cache.find(range);
        if (it != cache.end()) {
            memcpy(it->second->data(), buffer, aheadRange);
        } else {
            cache.insert(std::make_pair(range,
                new std::vector<uint8_t >(buffer, buffer + aheadRange)));
        }
    }

    uint32_t getRange(uint32_t sector) const {
        return sector * sectorSize / aheadRange;
    }

};

#endif //SOCKDRIVE_BLOCKCACHE_H
