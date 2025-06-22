//
// Created by caiii on 21.06.2025.
//

#include <protocol.h>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <unordered_map>

namespace {
  // TODO: download from s3
  std::string driveFolder = "/home/caiiiycuk/js-dos/dos.zone/unpacked/from-s3";
  std::unordered_map<uint32_t, std::vector<uint8_t>> ranges;

  uint8_t* readFileAsBytes(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
      printf("Failed to open file %s\n", filename.c_str());
      abort();
    }

    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    uint8_t* buffer = new uint8_t[size];
    file.read(reinterpret_cast<char*>(buffer), size);
    file.close();

    return buffer;
  }

}

void server_sockdrive_open(uint32_t handle, const char* address) {
  auto rangeCount = 8192;
  auto size = 2097152;
  auto heads = 128;
  auto cylinders = 520;
  auto sectors = 63;
  auto sectorSize = 512;
  auto aheadRange = 262144;

  std::string metafile = driveFolder + "/sockdrive.metas";
  std::ifstream infile(metafile);
  std::string line;

  if (!infile.is_open()) {
    printf("Failed to open meta file\n");
    abort();
  }

  if (!std::getline(infile, line)) {
    printf("Failed to read small ranges\n");
    abort();
  }

  std::vector<uint32_t> smallRanges;
  line = line.substr(14, line.size() - 14);
  {
    std::stringstream ss(line);
    std::string token;
  
    while (std::getline(ss, token, ',')) {
      smallRanges.push_back(std::stoul(token));
    }
  }

  if (!std::getline(infile, line)) {
    printf("Failed to read dropped ranges\n");
    abort();
  }

  std::vector<uint32_t> droppedRanges;
  line = line.substr(16, line.size() - 16);
  {
    std::stringstream ss(line);
    std::string token;
  
    while (std::getline(ss, token, ',')) {
      droppedRanges.push_back(std::stoul(token));
    }
  }
  
  infile.close();

  if (smallRanges.size() > 0) {
    auto preload = readFileAsBytes(driveFolder + "/preload.raw");
    for (int i = 0; i < smallRanges.size(); ++i) {
      ranges[smallRanges[i]] = std::vector<uint8_t>(preload + i * aheadRange, preload + (i + 1) * aheadRange);
    }
    delete[] preload;
  }

  for (uint32_t i = 0; i < rangeCount; ++i) {
    // TODO: optimize with maps
    if (std::find(smallRanges.begin(), smallRanges.end(), i) != smallRanges.end() ||
      std::find(droppedRanges.begin(), droppedRanges.end(), i) != droppedRanges.end()) {
      continue;
    }

    auto data = readFileAsBytes(driveFolder + "/" + std::to_string(i) + ".raw");
    ranges[i] = std::vector<uint8_t>(data, data + aheadRange);
    delete[] data;
  }

  auto emptyRangesCount = droppedRanges.size();;
  auto emptyRanges = new uint8_t[emptyRangesCount * 4];
  for (int i = 0; i < emptyRangesCount; ++i) {
    auto value = droppedRanges[i];
    auto offset = i * 4;
    emptyRanges[offset] = value & 0xFF;
    emptyRanges[offset + 1] = (value & 0x0000FF00) >> 8;
    emptyRanges[offset + 2] = (value & 0x00FF0000) >> 16;
    emptyRanges[offset + 3] = (value & 0xFF000000) >> 24;
  }
  client_sockdrive_opened(handle, size, heads, cylinders, sectors, sectorSize, aheadRange, emptyRangesCount, emptyRanges);
  delete[] emptyRanges;
}

extern void server_sockdrive_ready(uint32_t handle) {}
void server_sockdrive_close(uint32_t handle) {}
void server_sockdrive_load_range(uint32_t handle, uint32_t range) {
  auto it = ranges.find(range);
  if (it == ranges.end()) {
    printf("Range %u is not initialized\n", range);
    abort();
  }
  client_sockdrive_new_range(handle, range, it->second.data());
}
void server_sockdrive_write_sector(uint32_t handle, uint32_t sector, uint8_t* buffer) {}
