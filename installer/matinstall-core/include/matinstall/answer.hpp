#pragma once

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

namespace matinstall {

enum class Firmware { kUefi, kBios };
enum class RootFilesystem { kExt4, kBtrfs };

struct Answer {
  std::string disk;
  std::optional<std::uint64_t> disk_size_bytes;
  std::optional<std::uint32_t> logical_sector_size;
  Firmware firmware = Firmware::kUefi;
  RootFilesystem root_filesystem = RootFilesystem::kExt4;
  std::string source;
  std::string hostname;
  std::string username;
  std::string full_name;
  std::string password;
  std::string locale = "en_US.UTF-8";
  std::string timezone = "UTC";
  std::string keymap = "us";
  std::uint64_t swapfile_size_mib = 0;
  bool separate_home = false;
  std::optional<std::uint64_t> root_size_mib;
  std::optional<std::uint64_t> home_size_mib;
  bool destructive = false;
};

// Parses and fully validates an answer. ValidationError contains a useful,
// non-secret diagnostic. No probing or command execution is performed.
Answer parse_answer(std::string_view text);
Answer load_answer(const std::string& path);
std::string to_string(Firmware firmware);
std::string to_string(RootFilesystem filesystem);
bool syntactically_safe_disk_path(std::string_view path);

class ValidationError : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

}  // namespace matinstall
