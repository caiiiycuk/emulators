#include <jsdos-asyncify.h>
#include <protocol.h>
#include <sockdrive.h>

#include <string>
#include <unordered_set>

#include "./rangecache.h"

namespace {
struct SockDriveInstance {
  std::string url;
  uint32_t size;
  uint32_t heads;
  uint32_t cylinders;
  uint32_t sectors;
  uint32_t sectorSize;
  uint32_t aheadRange;
  std::unordered_set<uint32_t> emptyRanges;
};

std::unordered_map<uint32_t, std::unique_ptr<RangeCache>> driveCaches;
std::unordered_map<uint32_t, std::unordered_map<uint32_t, std::vector<uint8_t>>> writeCaches;
std::unordered_map<uint32_t, SockDriveInstance> drives;

bool await = false;
uint32_t lastHandle = 0;
std::pair<uint32_t, SockDriveInstance> openedInstance;

}  // namespace

void client_sockdrive_opened(uint32_t handle, uint32_t size, uint32_t heads, uint32_t cylinders, uint32_t sectors,
                             uint32_t sectorSize, uint32_t aheadRange, uint32_t emptyRangesCount,
                             uint8_t* emptyRanges) {
  if (!await) {
    printf("ERR! Unexpected sockrive_opened\n");
    abort();
  }
  std::unordered_set<uint32_t> emptyRangesSet(emptyRangesCount);
  for (uint32_t i = 0; i < emptyRangesCount; i++) {
    auto offset = i * 4;
    uint32_t value = (emptyRanges[offset] & 0x000000ff) | ((emptyRanges[offset + 1] << 8) & 0x0000ff00) |
                     ((emptyRanges[offset + 2] << 16) & 0x00ff0000) | ((emptyRanges[offset + 3] << 24) & 0xff000000);
    emptyRangesSet.insert(value);
  }
  openedInstance = std::make_pair<>(handle, SockDriveInstance{
                                                "",
                                                size,
                                                heads,
                                                cylinders,
                                                sectors,
                                                sectorSize,
                                                aheadRange,
                                                emptyRangesSet,
                                            });
  await = false;
}

void client_sockdrive_new_range(uint32_t handle, uint32_t range, uint8_t* buffer) {
  auto it = driveCaches.find(handle);
  if (it != driveCaches.end()) {
    it->second->create(range, buffer);
  } else {
    printf("ERR! Unexpected sockdrive_new_range, handle %d not exists\n", handle);
  }
}

uint32_t sockdrive_open(const char* url) {
  if (await) {
    printf("ERR! Already awaiting sockrive_open\n");
    abort();
  }
  lastHandle++;
  await = true;
  server_sockdrive_open(lastHandle, url);

  while (await) {
    asyncify_sleep(16, true);
  }

  openedInstance.second.url = url;
  if (openedInstance.second.size == 0) {
    return 0;
  }

  drives.insert(std::make_pair(openedInstance.first, openedInstance.second));
  driveCaches.insert(std::make_pair(
      openedInstance.first, new RangeCache(openedInstance.second.sectorSize, openedInstance.second.aheadRange)));
  writeCaches.insert(std::make_pair(openedInstance.first, std::unordered_map<uint32_t, std::vector<uint8_t>>()));
  server_sockdrive_ready(openedInstance.first);
  return openedInstance.first;
}

uint8_t sockdrive_read(uint32_t handle, uint32_t sector, uint8_t* buffer) {
  auto it = drives.find(handle);
  if (it != drives.end()) {
    auto& drive = it->second;
    auto it = writeCaches[handle].find(sector);
    if (it == writeCaches[handle].end()) {
      auto& cache = driveCaches[handle];
      auto range = cache->getRange(sector);
      if (drive.emptyRanges.find(range) != drive.emptyRanges.end()) {
        memset(buffer, 0, drive.sectorSize);
      } else {
        auto sectorPtr = cache->read(sector);
        if (!sectorPtr) {
          server_sockdrive_load_range(handle, range);
          while (!sectorPtr) {
            asyncify_sleep(0, true);
            sectorPtr = cache->read(sector);
          }
        }
        memcpy(buffer, sectorPtr, drive.sectorSize);
      }
    } else {
      memcpy(buffer, it->second.data(), drive.sectorSize);
    }
    return 0;
  } else {
    printf("ERR! Sockdrive %d not found!\n", handle);
  }
  return 1;
}

uint8_t sockdrive_write(uint32_t handle, uint32_t sector, uint8_t* buffer) {
  auto it = writeCaches.find(handle);
  if (it != writeCaches.end()) {
    it->second[sector] = std::vector<uint8_t>(buffer, buffer + drives[handle].sectorSize);
    server_sockdrive_write_sector(handle, sector, buffer);
    return 0;
  }
  return 1;
}

uint32_t sockdrive_size(uint32_t handle) {
  auto it = drives.find(handle);
  return it != drives.end() ? it->second.size : 0;
}

uint32_t sockdrive_heads(uint32_t handle) {
  auto it = drives.find(handle);
  return it != drives.end() ? it->second.heads : 0;
}

uint32_t sockdrive_sectors(uint32_t handle) {
  auto it = drives.find(handle);
  return it != drives.end() ? it->second.sectors : 0;
}

uint32_t sockdrive_cylinders(uint32_t handle) {
  auto it = drives.find(handle);
  return it != drives.end() ? it->second.cylinders : 0;
}

uint32_t sockdrive_sector_size(uint32_t handle) {
  auto it = drives.find(handle);
  return it != drives.end() ? it->second.sectorSize : 0;
}

void sockdrive_close(uint32_t handle) {
  server_sockdrive_close(handle);
  drives.erase(handle);
  driveCaches.erase(handle);
  writeCaches.erase(handle);
}