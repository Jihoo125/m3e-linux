#include "matinstall/answer.hpp"
#include "matinstall/json.hpp"
#include "matinstall/planner.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <limits>
#include <set>
#include <string_view>

namespace matinstall {
namespace {

const json::Value* get(const json::Value& object, std::string_view key) {
  return object.is_object() ? object.find(key) : nullptr;
}
const json::Value& required(const json::Value& object, std::string_view key) {
  const json::Value* value = get(object, key);
  if (value == nullptr) throw ValidationError("required field is missing: " + std::string(key));
  return *value;
}
std::string string_field(const json::Value& object, std::string_view key, bool mandatory = true) {
  const json::Value* value = get(object, key);
  if (value == nullptr || value->is_null()) {
    if (mandatory) throw ValidationError("required string field is missing: " + std::string(key));
    return {};
  }
  if (!value->is_string()) throw ValidationError(std::string(key) + " must be a string");
  return value->as_string();
}
bool bool_field(const json::Value& object, std::string_view key, bool fallback) {
  const json::Value* value = get(object, key);
  if (value == nullptr || value->is_null()) return fallback;
  if (!value->is_bool()) throw ValidationError(std::string(key) + " must be a boolean");
  return value->as_bool();
}
std::optional<std::uint64_t> integer_field(const json::Value& object, std::string_view key) {
  const json::Value* value = get(object, key);
  if (value == nullptr || value->is_null()) return std::nullopt;
  std::uint64_t result = 0;
  if (!json::number_to_u64(*value, result)) throw ValidationError(std::string(key) + " must be an unsigned integer");
  return result;
}
bool no_controls(std::string_view text) {
  return std::ranges::none_of(text, [](char c) { return static_cast<unsigned char>(c) < 0x20U || c == 0x7fU; });
}
bool ascii_identifier(std::string_view text, std::string_view allowed_punctuation, bool lowercase_first = false) {
  if (text.empty() || text.size() > 64) return false;
  const auto allowed = [&](char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
           allowed_punctuation.find(c) != std::string_view::npos;
  };
  return (lowercase_first ? ((text[0] >= 'a' && text[0] <= 'z') || text[0] == '_') : allowed(text[0])) &&
         std::ranges::all_of(text, allowed);
}

}  // namespace

std::string to_string(Firmware firmware) { return firmware == Firmware::kUefi ? "uefi" : "bios"; }
std::string to_string(RootFilesystem filesystem) { return filesystem == RootFilesystem::kExt4 ? "ext4" : "btrfs"; }

bool syntactically_safe_disk_path(std::string_view path) {
  if (path.size() < 7 || path.size() > 4096 || !path.starts_with("/dev/") || path.back() == '/') return false;
  if (path.find("//") != std::string_view::npos) return false;
  for (char c : path) {
    if (c == '/' || c == '.' || c == '_' || c == '-' || (c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) continue;
    return false;
  }
  std::size_t pos = 0;
  while (pos < path.size()) {
    const std::size_t slash = path.find('/', pos + 1);
    const std::size_t end = slash == std::string_view::npos ? path.size() : slash;
    const auto component = path.substr(pos + 1, end - pos - 1);
    if (component.empty() || component == "." || component == ".." || component.starts_with('-')) return false;
    if (slash == std::string_view::npos) break;
    pos = slash;
  }
  return true;
}

Answer parse_answer(std::string_view text) {
  const auto parsed = json::parse(text);
  if (!parsed.ok) throw ValidationError("invalid answer JSON at byte " + std::to_string(parsed.offset) + ": " + parsed.error);
  if (!parsed.value.is_object()) throw ValidationError("answer must be a JSON object");
  const auto& root = parsed.value;
  Answer answer;
  answer.disk = string_field(root, "disk");
  answer.disk_size_bytes = integer_field(root, "disk_size_bytes");
  if (auto value = integer_field(root, "logical_sector_size")) {
    if (*value > std::numeric_limits<std::uint32_t>::max()) throw ValidationError("logical_sector_size is too large");
    answer.logical_sector_size = static_cast<std::uint32_t>(*value);
  }
  const std::string firmware = string_field(root, "firmware", false);
  if (firmware.empty() || firmware == "uefi") answer.firmware = Firmware::kUefi;
  else if (firmware == "bios") answer.firmware = Firmware::kBios;
  else throw ValidationError("firmware must be uefi or bios");

  if (const auto* fs = get(root, "filesystem")) {
    if (!fs->is_object()) throw ValidationError("filesystem must be an object");
    const std::string type = string_field(*fs, "root", false);
    if (type.empty() || type == "ext4") answer.root_filesystem = RootFilesystem::kExt4;
    else if (type == "btrfs") answer.root_filesystem = RootFilesystem::kBtrfs;
    else throw ValidationError("filesystem.root must be ext4 or btrfs");
    answer.separate_home = bool_field(*fs, "separate_home", false);
  }
  answer.swapfile_size_mib = integer_field(root, "swapfile_mib").value_or(0);
  if (answer.swapfile_size_mib > 1024ULL * 1024ULL) throw ValidationError("swapfile_mib exceeds the supported 1 TiB maximum");
  answer.root_size_mib = integer_field(root, "root_size_mib");
  answer.home_size_mib = integer_field(root, "home_size_mib");
  if (answer.home_size_mib.has_value()) throw ValidationError("home_size_mib is not supported; home uses remaining space");
  if (!answer.separate_home && answer.root_size_mib.has_value()) throw ValidationError("root_size_mib requires filesystem.separate_home=true");
  if (answer.root_size_mib.has_value() && *answer.root_size_mib < kMinimumRootMiB) throw ValidationError("root_size_mib must be at least 8192");
  if (answer.root_filesystem == RootFilesystem::kBtrfs && answer.separate_home) throw ValidationError("Btrfs separate-home partitions are unsupported; Btrfs already has @home");

  answer.source = string_field(root, "source");
  answer.hostname = string_field(root, "hostname");
  answer.locale = string_field(root, "locale", false);
  if (answer.locale.empty()) answer.locale = "en_US.UTF-8";
  answer.timezone = string_field(root, "timezone", false);
  if (answer.timezone.empty()) answer.timezone = "UTC";
  answer.keymap = string_field(root, "keyboard", false);
  if (answer.keymap.empty()) answer.keymap = "us";
  const auto& user = required(root, "user");
  if (!user.is_object()) throw ValidationError("user must be an object");
  answer.username = string_field(user, "name");
  answer.full_name = string_field(user, "full_name", false);
  if (answer.full_name.empty()) answer.full_name = answer.username;
  answer.password = string_field(user, "password");
  answer.destructive = bool_field(root, "destructive", false);

  if (!syntactically_safe_disk_path(answer.disk)) throw ValidationError("disk must be a direct, safe whole-disk path below /dev");
  const auto basename = std::string_view(answer.disk).substr(answer.disk.rfind('/') + 1);
  if (basename.starts_with("loop") || basename.starts_with("ram") || basename.starts_with("zram") ||
      basename.starts_with("fd") || basename.starts_with("sr")) throw ValidationError("loop, optical, RAM and zram targets are forbidden");
  if (answer.source.empty() || answer.source.size() > 4096 || answer.source.front() != '/' || !no_controls(answer.source) || answer.source.find('\0') != std::string::npos) throw ValidationError("source must be an absolute path without control characters");
  if (answer.source.find("/../") != std::string::npos || answer.source.ends_with("/..") || answer.source.find("//") != std::string::npos) throw ValidationError("source cannot contain traversal or empty components");
  if (answer.source.starts_with("/dev/") || answer.source.starts_with("/proc/") || answer.source.starts_with("/sys/")) throw ValidationError("source must not point into a device or pseudo-filesystem namespace");

  if (answer.hostname.empty() || answer.hostname.size() > 253 || answer.hostname.front() == '.' || answer.hostname.back() == '.' || answer.hostname.front() == '-') throw ValidationError("invalid hostname");
  for (char c : answer.hostname) if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '.' || c == '-')) throw ValidationError("invalid hostname character");
  if (answer.hostname.find("..") != std::string::npos || answer.hostname.find(".-") != std::string::npos || answer.hostname.find("-.") != std::string::npos || answer.hostname.back() == '-') throw ValidationError("invalid hostname label");
  if (!ascii_identifier(answer.username, "_-", true) || answer.username.size() > 32) throw ValidationError("user.name must start lowercase/underscore and contain only lowercase letters, digits, _ or -");
  static const std::set<std::string, std::less<>> reserved{"root", "daemon", "bin", "sys", "sync", "nobody", "messagebus", "sshd", "_apt", "man", "lp", "mail", "news", "uucp", "proxy", "www-data", "backup", "list", "irc", "gnats", "systemd-network", "systemd-resolve", "materialized-live"};
  if (reserved.contains(answer.username)) throw ValidationError("user.name is reserved for a system account");
  if (answer.password.size() < 8 || answer.password.size() > 1024 || !no_controls(answer.password)) throw ValidationError("user.password must be 8–1024 bytes with no control characters");
  if (answer.full_name.empty() || answer.full_name.size() > 256 || !no_controls(answer.full_name) || answer.full_name.front() == '-' || answer.full_name.find_first_of(":,=") != std::string::npos) throw ValidationError("user.full_name has characters forbidden by passwd GECOS");
  if (!ascii_identifier(answer.keymap, "_-+", false)) throw ValidationError("keyboard must be a simple XKB layout identifier");
  if (answer.locale.empty() || answer.locale.size() > 127 || !std::ranges::all_of(answer.locale, [](char c) { return c > 0x20 && c < 0x7f; }) || answer.locale.find_first_of("/;=") != std::string::npos || answer.locale.find("..") != std::string::npos) throw ValidationError("locale must be a simple locale name such as en_US.UTF-8");
  if (answer.timezone.empty() || answer.timezone.size() > 255 || !no_controls(answer.timezone) || answer.timezone.front() == '/' || answer.timezone.front() == '-' || answer.timezone.find("..") != std::string::npos || answer.timezone.find("//") != std::string::npos) throw ValidationError("invalid timezone zoneinfo path");
  return answer;
}

Answer load_answer(const std::string& path) {
  std::ifstream stream(path, std::ios::binary | std::ios::ate);
  if (!stream) throw ValidationError("cannot read answer file: " + path);
  const std::streamoff length = stream.tellg();
  if (length < 0 || length > 1024 * 1024) throw ValidationError("answer file must be no larger than 1 MiB");
  std::string contents(static_cast<std::size_t>(length), '\0');
  stream.seekg(0);
  if (length > 0 && !stream.read(contents.data(), length)) throw ValidationError("cannot read answer file: " + path);
  return parse_answer(contents);
}

}  // namespace matinstall
