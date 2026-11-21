#include "matinstall/installer.hpp"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fcntl.h>
#include <limits>
#include <optional>
#include <set>
#include <string_view>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>

namespace matinstall {
namespace {

const Partition& require_partition(const PartitionPlan& plan, std::string_view role) {
  const Partition* value = find_partition(plan, role);
  if (value == nullptr) throw ValidationError("incomplete plan: missing " + std::string(role));
  return *value;
}

void add(std::vector<InstallStep>& steps, std::string name, Command command,
         bool destructive = false) {
  steps.emplace_back(std::move(name), std::move(command), destructive);
}

void mount(std::vector<InstallStep>& steps, std::string name, const std::string& device,
           const std::string& point, std::string type = {}, std::string options = {}) {
  Command command{{"mount"}};
  if (!type.empty()) { command.arguments.insert(command.arguments.end(), {"-t", std::move(type)}); }
  if (!options.empty()) { command.arguments.insert(command.arguments.end(), {"-o", std::move(options)}); }
  command.arguments.push_back(device);
  command.arguments.push_back(point);
  steps.emplace_back(std::move(name), std::move(command), false, point);
}

void target_dirs(std::vector<InstallStep>& steps, std::initializer_list<const char*> paths) {
  Command command{{"chroot", kTargetRoot, "mkdir", "-p", "--"}};
  for (const char* path : paths) command.arguments.emplace_back(path);
  add(steps, "Create directories inside target", std::move(command));
}

void bind_mount(std::vector<InstallStep>& steps, const std::string& directory, bool recursive) {
  const std::string destination = std::string(kTargetRoot) + directory;
  Command command{{"mount", recursive ? "--rbind" : "--bind", directory, destination}};
  steps.emplace_back("Bind host " + directory, std::move(command), false, destination);
  if (recursive) add(steps, "Prevent recursive-mount propagation", {{"mount", "--make-rslave", destination}});
  add(steps, "Verify bind mount", {{"findmnt", "--mountpoint", destination, "--noheadings", "--output", "TARGET"}});
}

std::string fstab_uuid(const std::string& device, const std::string& entry, bool truncate = false) {
  std::string script = "set -eu; UUID=$(blkid -s UUID -o value -- " + shell_escape(device) +
      "); case \"$UUID\" in ''|*[!A-Za-z0-9_-]*) exit 1 ;; esac; "
      "printf 'UUID=%s " + entry + "\\n' \"$UUID\" ";
  script += truncate ? ">" : ">>";
  return script + " /etc/fstab";
}

void target(std::vector<InstallStep>& steps, std::string name, std::string script, bool destructive = true) {
  add(steps, std::move(name), {{"chroot", kTargetRoot, "sh", "-c", std::move(script)}}, destructive);
}

std::vector<InstallStep> build_steps(const InstallationPlan& plan) {
  std::vector<InstallStep> steps;
  if (plan.answer.firmware == Firmware::kUefi) {
    // Refuse before the destructive first step if the live environment cannot
    // register a UEFI boot entry. The answer chooses the target firmware; no
    // host auto-detection or silent unregistered fallback is performed.
    add(steps, "partition: verify live UEFI efivars", {{"test", "-d", "/sys/firmware/efi/efivars"}});
  }
  Command sfdisk{{"sfdisk", "--wipe", "always", "--wipe-partitions", "always", "--", plan.disk.path}};
  sfdisk.standard_input = plan.partitions.sfdisk_script;
  add(steps, "partition: replace GPT partition table", std::move(sfdisk), true);
  add(steps, "partition: reload kernel's partition table", {{"partprobe", plan.disk.path}}, true);
  add(steps, "partition: settle udev", {{"udevadm", "settle"}});
  for (const auto& item : plan.partitions.partitions) {
    if (item.role == "bios_grub") continue;
    add(steps, "partition: verify device " + item.device, {{"test", "-b", item.device}});
    const std::string label = item.role == "efi" ? "MAT-EFI" :
                              item.role == "home" ? "materialized-home" : "MATERIALIZED";
    if (item.filesystem == "vfat") add(steps, "format: EFI FAT32", {{"mkfs.vfat", "-F", "32", "-n", label, item.device}}, true);
    else if (item.filesystem == "ext4") add(steps, "format: " + item.role + " ext4", {{"mkfs.ext4", "-F", "-L", label, item.device}}, true);
    else add(steps, "format: " + item.role + " Btrfs", {{"mkfs.btrfs", "-f", "-L", label, item.device}}, true);
  }

  const Partition& root = require_partition(plan.partitions, "root");
  const Partition* home = find_partition(plan.partitions, "home");
  const Partition* efi = find_partition(plan.partitions, "efi");
  const std::string& home_device = home == nullptr ? root.device : home->device;
  const bool btrfs = plan.answer.root_filesystem == RootFilesystem::kBtrfs;
  if (!btrfs) {
    mount(steps, "mount: root", root.device, kTargetRoot, "ext4");
    if (home != nullptr) {
      target_dirs(steps, {"/home"});
      mount(steps, "mount: separate ext4 home", home_device, std::string(kTargetRoot) + "/home", "ext4");
    }
  } else {
    mount(steps, "mount: Btrfs top-level", root.device, kTargetRoot, "btrfs");
    const std::string base = kTargetRoot;
    // Names intentionally match the mount subvol options and later fstab.
    for (const char* name : {"@home", "@snapshots", "@var_log"}) {
      add(steps, "create: Btrfs " + std::string(name), {{"btrfs", "subvolume", "create", base + "/" + name}});
    }
    add(steps, "create: Btrfs @", {{"btrfs", "subvolume", "create", base + "/@"}});
    if (plan.answer.swapfile_size_mib > 0) add(steps, "create: Btrfs @swap", {{"btrfs", "subvolume", "create", base + "/@swap"}});
    InstallStep unmount{"Unmount Btrfs top-level safely", {{"umount", kTargetRoot}}, false, {}, kTargetRoot};
    steps.push_back(std::move(unmount));
    mount(steps, "mount: Btrfs @", root.device, kTargetRoot, "btrfs", "subvol=@,compress=zstd");
    target_dirs(steps, {"/home", "/var", "/var/log", "/.snapshots"});
    mount(steps, "mount: Btrfs @home before deployment", root.device, std::string(kTargetRoot) + "/home", "btrfs", "subvol=@home,compress=zstd");
    mount(steps, "mount: Btrfs @var_log", root.device, std::string(kTargetRoot) + "/var/log", "btrfs", "subvol=@var_log,compress=zstd");
    mount(steps, "mount: read-only @snapshots", root.device, std::string(kTargetRoot) + "/.snapshots", "btrfs", "ro,subvol=@snapshots,compress=zstd");
    if (plan.answer.swapfile_size_mib > 0) {
      target_dirs(steps, {"/swap"});
      mount(steps, "mount: Btrfs @swap", root.device, std::string(kTargetRoot) + "/swap", "btrfs", "subvol=@swap,compress=zstd");
    }
  }
  if (efi != nullptr) {
    target_dirs(steps, {"/boot/efi"});
    mount(steps, "mount: EFI system partition", efi->device, std::string(kTargetRoot) + "/boot/efi", "vfat");
  }
  add(steps, "deploy: extract SquashFS image", {{"unsquashfs", "-f", "-d", kTargetRoot, plan.answer.source}});
  // Directories must be recreated after extraction covers mountpoint metadata.
  target_dirs(steps, {"/dev", "/proc", "/sys", "/run"});
  bind_mount(steps, "/dev", true);
  bind_mount(steps, "/sys", true);
  mount(steps, "mount: fresh proc", "proc", std::string(kTargetRoot) + "/proc", "proc");
  bind_mount(steps, "/run", false);
  if (plan.answer.firmware == Firmware::kUefi) {
    mount(steps, "mount: efivarfs for boot registration", "efivarfs",
          std::string(kTargetRoot) + "/sys/firmware/efi/efivars", "efivarfs");
    add(steps, "verify: efivarfs", {{"findmnt", "--mountpoint", std::string(kTargetRoot) + "/sys/firmware/efi/efivars", "--noheadings", "--output", "TARGET"}});
  }

  target(steps, "configure: root fstab", fstab_uuid(root.device, btrfs ? "/ btrfs defaults,subvol=@,compress=zstd 0 0" : "/ ext4 defaults 0 1", true));
  if (home != nullptr || btrfs) target(steps, "configure: home fstab", fstab_uuid(home_device, btrfs ? "/home btrfs defaults,subvol=@home,compress=zstd 0 2" : "/home ext4 defaults 0 2"));
  if (btrfs) {
    target(steps, "configure: log fstab", fstab_uuid(root.device, "/var/log btrfs defaults,subvol=@var_log,compress=zstd 0 2"));
    target(steps, "configure: read-only snapshots fstab", fstab_uuid(root.device, "/.snapshots btrfs ro,subvol=@snapshots,compress=zstd 0 2"));
  }
  if (efi != nullptr) target(steps, "configure: EFI fstab", fstab_uuid(efi->device, "/boot/efi vfat umask=0077 0 2"));
  if (plan.answer.swapfile_size_mib > 0) {
    const std::string file = btrfs ? "/swap/swapfile" : "/swapfile";
    if (btrfs) {
      target(steps, "configure: swap subvolume fstab", fstab_uuid(root.device, "/swap btrfs defaults,subvol=@swap,compress=zstd 0 0"));
      target(steps, "swap: Btrfs NOCOW before allocating", "truncate -s 0 " + file + "; chattr +C " + file + "; btrfs property set " + file + " compression none");
      // mkswap rejects unsupported Btrfs layouts, so incomplete allocation is a
      // hard failure rather than an assumption that swapon will work later.
      add(steps, "swap: allocate Btrfs file without holes", {{"chroot", kTargetRoot, "dd", "if=/dev/zero", "of=" + file, "bs=1M", "count=" + std::to_string(plan.answer.swapfile_size_mib), "conv=fsync", "status=none"}}, true);
    } else {
      add(steps, "swap: allocate ext4 file", {{"chroot", kTargetRoot, "fallocate", "--length", std::to_string(plan.answer.swapfile_size_mib) + "MiB", file}}, true);
    }
    add(steps, "swap: restrict permissions", {{"chroot", kTargetRoot, "chmod", "0600", file}}, true);
    add(steps, "swap: initialize", {{"chroot", kTargetRoot, "mkswap", "-f", file}}, true);
    target(steps, "configure: swapfile fstab", "printf '%s\\n' " + shell_escape(file + " none swap sw,nofail 0 0") + " >> /etc/fstab");
  }
  target(steps, "configure: validate timezone", "test -f /usr/share/zoneinfo/" + shell_escape(plan.answer.timezone));
  target(steps, "configure: timezone", "ln -sfn /usr/share/zoneinfo/" + shell_escape(plan.answer.timezone) + " /etc/localtime; printf '%s\\n' " + shell_escape(plan.answer.timezone) + " > /etc/timezone");
  target(steps, "configure: hostname", "printf '%s\\n' " + shell_escape(plan.answer.hostname) + " > /etc/hostname");
  target(steps, "configure: generate locale", "set -eu; touch /etc/locale.gen; grep -Fqx -- " + shell_escape(plan.answer.locale) + " /etc/locale.gen || printf '%s\\n' " + shell_escape(plan.answer.locale) + " >> /etc/locale.gen; locale-gen; printf 'LANG=%s\\n' " + shell_escape(plan.answer.locale) + " > /etc/default/locale");
  target(steps, "configure: keyboard", "umask 022; printf '%s\\n' " + shell_escape("XKBMODEL=pc105\nXKBLAYOUT=" + plan.answer.keymap + "\nXKBVARIANT=\nXKBOPTIONS=\nBACKSPACE=guess") + " > /etc/default/keyboard");
  add(steps, "user: create account and home", {{"chroot", kTargetRoot, "useradd", "--create-home", "--user-group", "--comment", plan.answer.full_name, "--shell", "/bin/bash", "--", plan.answer.username}}, true);
  add(steps, "user: add sudo administrative group", {{"chroot", kTargetRoot, "usermod", "--append", "--groups", "sudo", "--", plan.answer.username}}, true);
  Command password{{"chroot", kTargetRoot, "chpasswd"}};
  password.standard_input = plan.answer.username + ":" + plan.answer.password + "\n";
  password.sensitive_input = true;
  add(steps, "user: set password over protected stdin", std::move(password), true);
  target(steps, "user: lock direct root login", "passwd -l root");
  target(steps, "bootloader: update initramfs", "update-initramfs -u -k all || update-initramfs -c -k all");
  if (plan.answer.firmware == Firmware::kUefi) {
    add(steps, "bootloader: install GRUB for UEFI", {{"chroot", kTargetRoot, "grub-install",
        "--target=x86_64-efi", "--efi-directory=/boot/efi", "--bootloader-id=Materialized",
        "--recheck", "--no-floppy", root.device}}, true);
  } else {
    add(steps, "bootloader: install GRUB for BIOS", {{"chroot", kTargetRoot, "grub-install", "--target=i386-pc", "--recheck", "--no-floppy", plan.disk.path}}, true);
  }
  add(steps, "bootloader: generate GRUB configuration", {{"chroot", kTargetRoot, "update-grub"}}, true);
  add(steps, "finalize: sync data", {{"sync"}});
  return steps;
}

const json::Value& required(const json::Value& object, std::string_view key) {
  const auto* value = object.find(key);
  if (value == nullptr || value->is_null()) throw ValidationError("lsblk lacks field: " + std::string(key));
  return *value;
}
std::string string_field(const json::Value& object, std::string_view key) {
  const auto& value = required(object, key);
  if (!value.is_string()) throw ValidationError("lsblk field must be a string: " + std::string(key));
  return value.as_string();
}
bool bool_field(const json::Value& object, std::string_view key) {
  const auto& value = required(object, key);
  if (!value.is_bool()) throw ValidationError("lsblk field must be boolean: " + std::string(key));
  return value.as_bool();
}
template <typename T> T number_field(const json::Value& object, std::string_view key) {
  std::uint64_t value = 0;
  if (!json::number_to_u64(required(object, key), value) || value > std::numeric_limits<T>::max()) throw ValidationError("lsblk numeric field invalid: " + std::string(key));
  return static_cast<T>(value);
}
void inspect_usage(const json::Value& row, bool& in_use) {
  if (!row.is_object()) throw ValidationError("invalid lsblk row");
  const auto& mounts = required(row, "mountpoints");
  if (!mounts.is_array()) throw ValidationError("lsblk mountpoints must be an array");
  for (const auto& value : mounts.as_array()) if (!value.is_null()) in_use = true;
  const std::string numbers = string_field(row, "maj:min");
  const auto separator = numbers.find(':');
  if (separator == std::string::npos || separator == 0 || separator + 1 == numbers.size() ||
      numbers.find(':', separator + 1) != std::string::npos ||
      !std::ranges::all_of(numbers, [](char c) { return c == ':' || (c >= '0' && c <= '9'); })) {
    throw ValidationError("invalid MAJ:MIN in lsblk output");
  }
  const std::string holder_path = "/sys/dev/block/" + numbers + "/holders";
  std::error_code ec;
  const auto holder_status = std::filesystem::status(holder_path, ec);
  if (ec || !std::filesystem::is_directory(holder_status)) throw ValidationError("cannot verify holders for " + string_field(row, "path"));
  const auto entries = std::filesystem::directory_iterator(holder_path, ec);
  if (ec || entries != std::filesystem::directory_iterator{}) in_use = true;
  if (const auto* children = row.find("children")) {
    if (!children->is_array()) throw ValidationError("lsblk children must be an array");
    for (const auto& child : children->as_array()) inspect_usage(child, in_use);
  }
}

struct LiveDisk { Disk disk; bool in_use; };
LiveDisk inspect_live_disk(const std::string& path, ProcessRunner& runner) {
  if (!syntactically_safe_disk_path(path)) throw ValidationError("unsafe disk path");
  struct stat status {};
  if (::lstat(path.c_str(), &status) != 0 || !S_ISBLK(status.st_mode)) throw ValidationError("not an existing direct block device (symlinks are not accepted)");
  const ProcessResult probe = runner.run({{"lsblk", "--json", "--bytes", "--paths", "--tree", "--output", "PATH,TYPE,SIZE,LOG-SEC,RO,MAJ:MIN,MOUNTPOINTS", path}});
  if (!probe.succeeded()) throw ValidationError("disk probe failed: " + probe.standard_error);
  const auto parsed = json::parse(probe.standard_output);
  if (!parsed.ok) throw ValidationError("lsblk JSON parsing failed: " + parsed.error);
  const auto* devices = parsed.value.find("blockdevices");
  if (devices == nullptr || !devices->is_array() || devices->as_array().size() != 1) throw ValidationError("expected exactly one selected disk");
  const auto& row = devices->as_array()[0];
  if (string_field(row, "path") != path || string_field(row, "type") != "disk") throw ValidationError("refusing target that is not an exact whole-disk lsblk object");
  if (path.starts_with("/dev/mapper/")) throw ValidationError("device-mapper targets are not supported");
  Disk disk{path, number_field<std::uint64_t>(row, "size"), number_field<std::uint32_t>(row, "log-sec"), bool_field(row, "ro"), false, true};
  bool in_use = false;
  inspect_usage(row, in_use);
  return {disk, in_use};
}

class FileLog {
 public:
  ~FileLog() { if (fd_ >= 0) ::close(fd_); }
  void open(const std::string& path) {
    fd_ = ::open(path.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
    if (fd_ < 0) throw ValidationError("cannot exclusively create log file: " + path);
  }
  void write(const std::string& line) {
    const std::string text = line + '\n';
    std::size_t offset = 0;
    while (offset < text.size()) {
      const ssize_t result = ::write(fd_, text.data() + offset, text.size() - offset);
      if (result > 0) { offset += static_cast<std::size_t>(result); continue; }
      if (result < 0 && errno == EINTR) continue;
      failed_ = true;
      return;
    }
  }
  bool failed() const { return failed_; }
 private:
  int fd_ = -1;
  bool failed_ = false;
};

std::string default_log_path() {
  const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(
      std::chrono::system_clock::now().time_since_epoch()).count();
  return "/var/log/materialized/install-" + std::to_string(seconds) + "-" + std::to_string(::getpid()) + ".log";
}

void prepare_target_directory() {
  std::error_code ec;
  const auto status = std::filesystem::symlink_status(kTargetRoot, ec);
  if (!ec && std::filesystem::exists(status)) {
    if (!std::filesystem::is_directory(status) || std::filesystem::is_symlink(status)) throw ValidationError("target mountpoint cannot be a symlink or non-directory");
    if (::rmdir(kTargetRoot) != 0) throw ValidationError("existing target mountpoint must be empty");
  }
  std::filesystem::create_directories(kTargetRoot, ec);
  if (ec) throw ValidationError("cannot create target mountpoint");
}

}  // namespace

std::string probe_lsblk(ProcessRunner& runner) {
  const auto result = runner.run({{"lsblk", "--json", "--bytes", "--paths", "--tree", "--output", "NAME,PATH,MAJ:MIN,TYPE,SIZE,LOG-SEC,RO,RM,MODEL,SERIAL,VENDOR,FSTYPE,LABEL,UUID,MOUNTPOINTS"}});
  if (!result.succeeded()) throw ValidationError("lsblk failed: " + result.standard_error);
  const auto parsed = json::parse(result.standard_output);
  if (!parsed.ok || !parsed.value.is_object() || parsed.value.find("blockdevices") == nullptr || !parsed.value.find("blockdevices")->is_array()) throw ValidationError("invalid lsblk probe output");
  return json::stringify(parsed.value);
}
Disk inspect_disk(const std::string& path, ProcessRunner& runner) {
  LiveDisk inspected = inspect_live_disk(path, runner);
  inspected.disk.in_use = inspected.in_use;
  return std::move(inspected.disk);
}

InstallationPlan make_installation_plan(const Answer& answer, const Disk& disk) {
  if (answer.swapfile_size_mib > 1024ULL * 1024ULL) {
    throw ValidationError("swapfile_mib exceeds the supported 1 TiB maximum");
  }
  InstallationPlan plan;
  plan.answer = answer;
  plan.disk = disk;
  plan.partitions = plan_partitions(answer, disk);
  const Partition& root = require_partition(plan.partitions, "root");
  if (root.size_mib < kMinimumRootMiB ||
      answer.swapfile_size_mib > root.size_mib - kMinimumRootMiB) {
    throw ValidationError("root partition must retain 8192 MiB after the requested swapfile");
  }
  plan.steps = build_steps(plan);
  if (answer.root_filesystem == RootFilesystem::kBtrfs && answer.swapfile_size_mib > 0) plan.warnings.push_back("Btrfs swap uses allocated NOCOW @swap storage; hibernation is not configured");
  return plan;
}

json::Value installation_plan_to_json(const InstallationPlan& plan) {
  auto output = json::Value::object();
  output.set("schema_version", json::Value::number(std::int64_t{1}));
  output.set("dry_run", true);
  output.set("disk", plan.disk.path);
  output.set("disk_size_bytes", json::Value::number(plan.disk.size_bytes));
  output.set("logical_sector_size", json::Value::number(static_cast<std::uint64_t>(plan.disk.logical_sector_size)));
  output.set("disk_probed", plan.disk.probed);
  output.set("firmware", to_string(plan.answer.firmware));
  output.set("filesystem", to_string(plan.answer.root_filesystem));
  output.set("target_root", kTargetRoot);
  auto partitions = json::Value::array();
  for (const auto& item : plan.partitions.partitions) {
    auto row = json::Value::object();
    row.set("number", json::Value::number(item.number));
    row.set("role", item.role);
    row.set("device", item.device);
    row.set("start_sector", json::Value::number(item.start_sector));
    row.set("size_sectors", json::Value::number(item.size_sectors));
    row.set("size_mib", json::Value::number(item.size_mib));
    row.set("type_uuid", item.type_uuid);
    row.set("name", item.name);
    row.set("filesystem", item.filesystem);
    row.set("mount_point", item.mount_point);
    partitions.push(std::move(row));
  }
  output.set("partitions", std::move(partitions));
  output.set("sfdisk_script", plan.partitions.sfdisk_script);
  auto commands = json::Value::array();
  for (const auto& step : plan.steps) {
    auto row = json::Value::object();
    row.set("name", step.name);
    row.set("command", format_command(step.command));
    row.set("destructive", step.destructive);
    if (!step.command.standard_input.empty()) row.set("stdin", "<redacted>");
    commands.push(std::move(row));
  }
  output.set("commands", std::move(commands));
  output.set("confirmation_required", "destructive:true and --i-understand-this-erases-data");
  output.set("secrets_redacted", true);
  auto warnings = json::Value::array();
  for (const auto& warning : plan.warnings) warnings.push(warning);
  output.set("warnings", std::move(warnings));
  return output;
}

ExecutionReport execute_installation(const Answer& answer, bool confirmation_flag,
                                     ProcessRunner& runner, const std::string& requested_log_file) {
  ExecutionReport report;
  report.log_file = requested_log_file.empty() ? default_log_path() : requested_log_file;
  if (!answer.destructive || !confirmation_flag) {
    report.error = !answer.destructive ? "run requires destructive:true in the answer"
                                      : "run requires --i-understand-this-erases-data";
    return report;
  }
  FileLog log;
  try {
    if (::geteuid() != 0) throw ValidationError("root is required; invoke through the polkit system daemon");
    std::error_code ec;
    const auto source_status = std::filesystem::symlink_status(answer.source, ec);
    if (ec || !std::filesystem::is_regular_file(source_status) || std::filesystem::is_symlink(source_status) || ::access(answer.source.c_str(), R_OK) != 0) throw ValidationError("source must be a readable regular SquashFS file, not a symlink");
    const LiveDisk live = inspect_live_disk(answer.disk, runner);
    if (live.disk.read_only || live.in_use) throw ValidationError("disk is read-only, mounted, or has active holders");
    const InstallationPlan plan = make_installation_plan(answer, live.disk);

    const ProcessResult target_mount = runner.run({{"findmnt", "--mountpoint", kTargetRoot, "--noheadings", "--output", "TARGET"}});
    if (target_mount.exit_code != 1 || !target_mount.error.empty()) throw ValidationError("target is mounted or mount state cannot be determined");
    prepare_target_directory();
    std::set<std::string> tools;
    for (const auto& step : plan.steps) tools.insert(step.command.arguments.front());
    for (const auto& tool : tools) {
      // Check the exact executable name using the trusted-path resolver shared
      // by SystemProcessRunner. The injected ProcessRunner may be a unit fake,
      // so this is a separate host preflight, not a fake command execution.
      if (!host_tool_exists(tool)) throw ValidationError("required host tool is missing: " + tool);
    }
    const ProcessResult squash = runner.run({{"unsquashfs", "-s", answer.source}});
    if (!squash.succeeded()) throw ValidationError("invalid SquashFS source: " + squash.standard_error);

    std::filesystem::create_directories("/var/log/materialized", ec);
    if (ec) throw ValidationError("cannot create log directory");
    ::chmod("/var/log/materialized", 0700);
    log.open(report.log_file);
    auto sink = [&](const std::string& line) { log.write(line); };
    log.write("START whole-disk installation; target=" + answer.disk);
    StepRunReport result = StepRunner(runner, sink).run(plan.steps);
    report.success = result.success && !log.failed();
    report.error = result.error;
    if (log.failed()) report.error += "; installation log write failed";
    if (result.success) log.write("SUCCESS all commands completed; tracked mounts removed");
    else log.write("ERROR " + result.error);
  } catch (const std::exception& error) {
    report.success = false;
    report.error = error.what();
    log.write("FATAL " + report.error);
  }
  return report;
}

}  // namespace matinstall
