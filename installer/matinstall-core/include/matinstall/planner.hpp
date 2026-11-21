#pragma once

#include "matinstall/answer.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace matinstall {

inline constexpr std::uint64_t kMiB = 1024ULL * 1024ULL;
inline constexpr std::uint64_t kMinimumRootMiB = 8192;
inline constexpr std::uint64_t kMinimumHomeMiB = 1024;
inline constexpr std::uint64_t kEfiSizeMiB = 512;
inline constexpr std::uint64_t kBiosBootSizeMiB = 1;

struct Disk {
  std::string path;
  std::uint64_t size_bytes = 0;
  std::uint32_t logical_sector_size = 512;
  bool read_only = false;
  bool in_use = false;
  bool probed = false;
};

struct Partition {
  std::size_t number = 0;
  std::string role;
  std::string device;
  std::string type_uuid;
  std::string name;
  std::uint64_t start_sector = 0;
  std::uint64_t size_sectors = 0;
  std::uint64_t start_mib = 0;
  std::uint64_t size_mib = 0;
  std::string filesystem;
  std::string mount_point;
};

struct PartitionPlan {
  Disk disk;
  Firmware firmware = Firmware::kUefi;
  RootFilesystem root_filesystem = RootFilesystem::kExt4;
  std::uint64_t swapfile_size_mib = 0;
  bool separate_home = false;
  std::vector<Partition> partitions;
  std::string sfdisk_script;
};

// Pure geometry/planning code. It neither probes devices nor executes tools.
PartitionPlan plan_partitions(const Answer& answer, const Disk& disk);
const Partition* find_partition(const PartitionPlan& plan, std::string_view role);
std::string partition_device(std::string_view disk, std::size_t number);

}  // namespace matinstall
