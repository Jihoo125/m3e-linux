#include "matinstall/planner.hpp"

#include "matinstall/json.hpp"

#include <algorithm>
#include <limits>
#include <sstream>

namespace matinstall {
namespace {

constexpr const char* kEfiType = "C12A7328-F81F-11D2-BA4B-00A0C93EC93B";
constexpr const char* kBiosType = "21686148-6449-6E6F-744E-656564454649";
constexpr const char* kLinuxType = "0FC63DAF-8483-4772-8E79-3D69D8477DE4";

std::uint64_t floor_to(std::uint64_t value, std::uint64_t multiple) {
  return value / multiple * multiple;
}

std::uint64_t ceil_to(std::uint64_t value, std::uint64_t multiple) {
  if (value > std::numeric_limits<std::uint64_t>::max() - (multiple - 1)) {
    throw ValidationError("partition geometry overflows a 64-bit sector count");
  }
  return floor_to(value + multiple - 1, multiple);
}

std::uint64_t mib_to_sectors(std::uint64_t mib, std::uint32_t sector_size) {
  if (mib > std::numeric_limits<std::uint64_t>::max() / kMiB) {
    throw ValidationError("partition size overflows byte arithmetic");
  }
  return mib * kMiB / sector_size;
}

std::string sfdisk_quote(std::string_view value) {
  std::string result = "\"";
  for (const char c : value) {
    if (c == '\\' || c == '"') result.push_back('\\');
    result.push_back(c);
  }
  result.push_back('"');
  return result;
}

void append_partition(PartitionPlan& plan, Disk disk, std::uint64_t& cursor,
                      std::string role, std::uint64_t size_sectors, std::uint64_t alignment,
                      std::string type, std::string name, std::string filesystem,
                      std::string mount_point, bool remainder = false) {
  const std::uint64_t start = ceil_to(cursor, alignment);
  if (remainder) {
    size_sectors = disk.size_bytes / disk.logical_sector_size - 33 - start;
  }
  const std::uint64_t end = start + size_sectors;  // exclusive
  if (size_sectors == 0 || end < start || end > disk.size_bytes / disk.logical_sector_size - 33) {
    throw ValidationError("partition " + role + " exceeds usable disk capacity");
  }
  const std::size_t number = plan.partitions.size() + 1;
  Partition partition;
  partition.number = number;
  partition.role = std::move(role);
  partition.device = partition_device(disk.path, number);
  partition.type_uuid = std::move(type);
  partition.name = std::move(name);
  partition.start_sector = start;
  partition.size_sectors = size_sectors;
  partition.start_mib = start * disk.logical_sector_size / kMiB;
  partition.size_mib = size_sectors * disk.logical_sector_size / kMiB;
  partition.filesystem = std::move(filesystem);
  partition.mount_point = std::move(mount_point);
  plan.partitions.push_back(std::move(partition));
  cursor = end;
}

}  // namespace

std::string partition_device(std::string_view disk, std::size_t number) {
  // Linux's addpart naming convention: a trailing digit requires a separating p.
  // /dev/mapper/* is rejected by the whole-disk execution preflight because its
  // kernel naming and DM dependencies require separate handling.
  const bool add_p = !disk.empty() && disk.back() >= '0' && disk.back() <= '9';
  return std::string(disk) + (add_p ? "p" : "") + std::to_string(number);
}

const Partition* find_partition(const PartitionPlan& plan, std::string_view role) {
  const auto found = std::ranges::find_if(plan.partitions,
      [role](const Partition& partition) { return partition.role == role; });
  return found == plan.partitions.end() ? nullptr : &*found;
}

PartitionPlan plan_partitions(const Answer& answer, const Disk& disk) {
  if (!syntactically_safe_disk_path(disk.path) || disk.path != answer.disk) {
    throw ValidationError("planner disk path must match the validated answer disk");
  }
  if (disk.logical_sector_size < 512 || disk.logical_sector_size > 4096 ||
      (disk.logical_sector_size & (disk.logical_sector_size - 1U)) != 0U) {
    throw ValidationError("logical sector size must be 512, 1024, 2048, or 4096 bytes");
  }
  if (disk.size_bytes < disk.logical_sector_size * 68ULL ||
      disk.size_bytes % disk.logical_sector_size != 0) {
    throw ValidationError("disk is too small or is not aligned to its logical sector size");
  }
  if (answer.disk_size_bytes.has_value() && answer.disk_size_bytes.value() != disk.size_bytes) {
    throw ValidationError("answer disk_size_bytes does not match the supplied/probed disk size");
  }
  if (answer.logical_sector_size.has_value() &&
      answer.logical_sector_size.value() != disk.logical_sector_size) {
    throw ValidationError("answer logical_sector_size does not match the supplied/probed disk");
  }
  if (answer.disk_size_bytes.has_value() && answer.disk_size_bytes.value() % disk.logical_sector_size != 0) {
    throw ValidationError("disk_size_bytes must be an exact multiple of the logical sector size");
  }

  const std::uint64_t total_sectors = disk.size_bytes / disk.logical_sector_size;
  const std::uint64_t alignment = std::max<std::uint64_t>(mib_to_sectors(1, disk.logical_sector_size), 8);
  if (total_sectors <= 34 + alignment) throw ValidationError("disk has no usable GPT data area");

  PartitionPlan plan;
  plan.disk = disk;
  plan.firmware = answer.firmware;
  plan.root_filesystem = answer.root_filesystem;
  plan.swapfile_size_mib = answer.swapfile_size_mib;
  plan.separate_home = answer.separate_home;
  std::uint64_t cursor = 34;
  if (answer.firmware == Firmware::kUefi) {
    append_partition(plan, disk, cursor, "efi", mib_to_sectors(kEfiSizeMiB, disk.logical_sector_size),
                     alignment, kEfiType, "EFI system partition", "vfat", "/boot/efi");
  } else {
    // 1 MiB alignment wastes slightly more than the conventional first MiB for
    // bios_grub, but leaves a documented, conservative MBR gap for GRUB.
    append_partition(plan, disk, cursor, "bios_grub", mib_to_sectors(kBiosBootSizeMiB, disk.logical_sector_size),
                     alignment, kBiosType, "BIOS boot partition", "", "");
  }

  const std::uint64_t bytes_before_payload = cursor * disk.logical_sector_size;
  const std::uint64_t usable_bytes = disk.size_bytes - 33ULL * disk.logical_sector_size - bytes_before_payload;
  if (!answer.separate_home) {
    if (usable_bytes / kMiB < kMinimumRootMiB) {
      throw ValidationError("disk is too small: root needs at least 8192 MiB after boot partitions");
    }
  } else {
    const std::uint64_t root_size = answer.root_size_mib.value_or(usable_bytes / kMiB / 2);
    if (root_size < kMinimumRootMiB) throw ValidationError("root partition needs at least 8192 MiB");
    const std::uint64_t root_sectors = mib_to_sectors(root_size, disk.logical_sector_size);
    if (root_sectors > total_sectors - 33 - cursor) {
      throw ValidationError("requested root partition does not fit on disk");
    }
    append_partition(plan, disk, cursor, "root", root_sectors, alignment, kLinuxType,
                     "Materialized root", to_string(answer.root_filesystem), "/");
    const std::uint64_t home_available = total_sectors - 33 - cursor;
    if (home_available == 0 ||
        home_available * disk.logical_sector_size / kMiB < kMinimumHomeMiB) {
      throw ValidationError("no room remains for a separate home partition of at least 1024 MiB");
    }
  }

  append_partition(plan, disk, cursor, answer.separate_home ? "home" : "root", 0, alignment,
                   kLinuxType, answer.separate_home ? "Materialized home" : "Materialized root",
                   to_string(answer.root_filesystem), answer.separate_home ? "/home" : "/", true);

  std::ostringstream script;
  script << "# matinstall-core GPT; destructive: replaces the entire disk label\n";
  script << "label: gpt\n";
  script << "unit: sectors\n\n";
  for (const Partition& partition : plan.partitions) {
    script << "start=" << partition.start_sector << ", size=" << partition.size_sectors
           << ", type=" << partition.type_uuid << ", name=" << sfdisk_quote(partition.name) << "\n";
  }
  plan.sfdisk_script = script.str();
  return plan;
}

}  // namespace matinstall
