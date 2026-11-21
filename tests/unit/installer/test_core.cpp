#include "matinstall/answer.hpp"
#include "matinstall/installer.hpp"
#include "matinstall/json.hpp"
#include "matinstall/planner.hpp"
#include "matinstall/process.hpp"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace matinstall;

namespace {

int failures = 0;

#define CHECK(condition)                                                                     \
  do {                                                                                       \
    if (!(condition)) {                                                                      \
      std::cerr << __FILE__ << ":" << __LINE__ << ": CHECK failed: " #condition "\n";      \
      ++failures;                                                                            \
    }                                                                                        \
  } while (false)

#define REQUIRE(condition)                                                                   \
  do {                                                                                       \
    if (!(condition)) {                                                                      \
      std::cerr << __FILE__ << ":" << __LINE__ << ": REQUIRE failed: " #condition "\n";    \
      throw std::runtime_error("test precondition failed");                                  \
    }                                                                                        \
  } while (false)

class FakeRunner final : public ProcessRunner {
 public:
  std::vector<Command> calls;
  std::vector<ProcessResult> responses;

  ProcessResult run(const Command& command) override {
    calls.push_back(command);
    if (responses.empty()) return ProcessResult{};
    ProcessResult result = std::move(responses.front());
    responses.erase(responses.begin());
    return result;
  }
};

std::string base_answer() {
  return R"json({
    "disk":"/dev/sda", "disk_size_bytes":128849018880, "firmware":"uefi",
    "filesystem":{"root":"ext4"}, "source":"/cdrom/filesystem.squashfs",
    "hostname":"materialized", "locale":"en_US.UTF-8", "timezone":"UTC", "keyboard":"us",
    "user":{"name":"iris","full_name":"Iris Example","password":"not-a-real-test-password"}
  })json";
}

Answer answer(const std::string& text = {}) { return parse_answer(text.empty() ? base_answer() : text); }

void expect_invalid(const std::string& json, std::string_view substring = {}) {
  try {
    (void)parse_answer(json);
    CHECK(false);
  } catch (const ValidationError& error) {
    if (!substring.empty()) CHECK(std::string_view(error.what()).find(substring) != std::string_view::npos);
  }
}

Disk disk(std::uint64_t size, std::string path = "/dev/sda", std::uint32_t sector = 512) {
  return Disk{std::move(path), size, sector, false, false, false};
}

void contains_command(const InstallationPlan& plan, std::string_view needle) {
  CHECK(std::ranges::any_of(plan.steps, [&](const InstallStep& step) {
    return format_command(step.command).find(needle) != std::string::npos;
  }));
}

bool has_secret(const InstallationPlan& plan, const std::string& secret) {
  return json::stringify(installation_plan_to_json(plan)).find(secret) != std::string::npos;
}

void test_json_parser() {
  const auto parsed = json::parse(R"({"disk":"\\dev\tsda","size":18446744073709551615,"u":"\uD83D\uDE00","a":[true,false,null]})");
  REQUIRE(parsed.ok);
  REQUIRE(parsed.value.is_object());
  CHECK(parsed.value.find("disk")->as_string() == "\\dev\tsda");
  std::uint64_t integer = 0;
  CHECK(json::number_to_u64(*parsed.value.find("size"), integer));
  CHECK(integer == std::numeric_limits<std::uint64_t>::max());
  CHECK(json::stringify(parsed.value).find("18446744073709551615") != std::string::npos);
  CHECK(json::parse("{\"a\":1,\"a\":2}").ok == false);
  CHECK(json::parse("01").ok == false);
  CHECK(json::parse("\"\\x\"").ok == false);
  CHECK(json::parse("[\"unterminated").ok == false);
  std::string invalid_utf8 = "\"";
  invalid_utf8.push_back(static_cast<char>(0xc0));
  invalid_utf8.push_back(static_cast<char>(0x80));
  invalid_utf8.push_back('"');
  CHECK(json::parse(invalid_utf8).ok == false);
}

void test_answer_validation() {
  auto value = answer();
  CHECK(value.disk == "/dev/sda");
  CHECK(value.firmware == Firmware::kUefi);
  CHECK(value.root_filesystem == RootFilesystem::kExt4);
  CHECK(!value.destructive);
  CHECK(value.full_name == "Iris Example");
  expect_invalid("{");
  expect_invalid("[]");
  expect_invalid("{}");
  expect_invalid(R"({"disk":"/tmp/sda","source":"/image.squashfs","hostname":"host","user":{"name":"user","password":"password1"}})", "/dev");
  expect_invalid(R"({"disk":"/dev/sda","source":"/image.squashfs","hostname":"host","user":{"name":"root","password":"password1"}})", "reserved");
  expect_invalid(R"({"disk":"/dev/../etc/sda","source":"/image.squashfs","hostname":"host","user":{"name":"user","password":"password1"}})");
  expect_invalid(R"({"disk":"/dev/sda","source":"/image.squashfs","hostname":"-bad","user":{"name":"user","password":"password1"}})");
  expect_invalid(R"({"disk":"/dev/sda","source":"/image.squashfs","hostname":"host","user":{"name":"user;whoami","password":"password1"}})");
  expect_invalid(R"({"disk":"/dev/sda","source":"/image.squashfs","hostname":"host","user":{"name":"user","password":"short"}})");
  expect_invalid(R"({"disk":"/dev/sda","source":"/../image","hostname":"host","user":{"name":"user","password":"password1"}})");
  expect_invalid(R"({"disk":"/dev/sda","source":"/image","hostname":"host","user":{"name":"user","password":"password1"},"root_size_mib":10})", "separate_home");
}

void test_uefi_geometry() {
  const auto plan = plan_partitions(answer(), disk(120ULL * 1024 * kMiB));
  REQUIRE(plan.partitions.size() == 2);
  const auto& efi = plan.partitions[0];
  const auto& root = plan.partitions[1];
  CHECK(efi.number == 1);
  CHECK(efi.role == "efi");
  CHECK(efi.start_sector == 2048);
  CHECK(efi.size_mib == 512);
  CHECK(efi.start_sector % 2048 == 0);
  CHECK(root.number == 2);
  CHECK(root.start_sector == efi.start_sector + efi.size_sectors);
  CHECK(root.start_sector % 2048 == 0);
  CHECK(root.mount_point == "/");
  CHECK(root.filesystem == "ext4");
  CHECK(root.device == "/dev/sda2");
  CHECK(root.start_sector + root.size_sectors == plan.disk.size_bytes / 512 - 33);
  const auto end = root.start_sector + root.size_sectors + 33;
  CHECK(end == plan.disk.size_bytes / 512);
  CHECK(plan.sfdisk_script.find("label: gpt") != std::string::npos);
  CHECK(plan.sfdisk_script.find("C12A7328-F81F-11D2-BA4B-00A0C93EC93B") != std::string::npos);
}

void test_bios_geometry() {
  auto value = answer(R"({"disk":"/dev/sdb","firmware":"bios","filesystem":{"root":"btrfs"},"source":"/filesystem.squashfs","hostname":"host","user":{"name":"user","password":"password"}})");
  const auto plan = plan_partitions(value, disk(40ULL * 1024 * kMiB, "/dev/sdb"));
  REQUIRE(plan.partitions.size() == 2);
  CHECK(plan.partitions[0].role == "bios_grub");
  CHECK(plan.partitions[0].size_mib == 1);
  CHECK(plan.partitions[0].type_uuid == "21686148-6449-6E6F-744E-656564454649");
  CHECK(plan.partitions[1].filesystem == "btrfs");
  CHECK(plan.partitions[1].start_sector % 2048 == 0);
  CHECK(plan.sfdisk_script.find("type=21686148") != std::string::npos);
}

void test_separate_home() {
  auto value = answer(R"({"disk":"/dev/nvme0n1","firmware BIOS":"bios","firmware":"uefi","filesystem":{"root":"ext4","separate_home":true},"root_size_mib":10240,"source":"/filesystem.squashfs","hostname":"host","user":{"name":"user","password":"password"}})");
  auto plan = plan_partitions(value, disk(64ULL * 1024 * kMiB, "/dev/nvme0n1"));
  REQUIRE(plan.partitions.size() == 3);
  CHECK(plan.partitions[0].device == "/dev/nvme0n1p1");
  CHECK(plan.partitions[1].device == "/dev/nvme0n1p2");
  CHECK(plan.partitions[2].device == "/dev/nvme0n1p3");
  CHECK(plan.partitions[1].size_mib == 10240);
  CHECK(plan.partitions[2].role == "home");
  CHECK(plan.partitions[2].mount_point == "/home");
  CHECK(plan.partitions[2].size_mib > 1024);
  REQUIRE(find_partition(plan, "home") != nullptr);
  CHECK(find_partition(plan, "missing") == nullptr);
  value.root_size_mib = 1024;
  try {
    (void)plan_partitions(value, disk(64ULL * 1024 * kMiB, "/dev/nvme0n1"));
    CHECK(false);
  } catch (const ValidationError&) {}
}

void test_4k_alignment() {
  auto value = answer(R"({"disk":"/dev/sdb","disk_size_bytes":34359738368,"logical_sector_size":4096,"source":"/filesystem.squashfs","hostname":"host","user":{"name":"user","password":"password"}})");
  const auto plan = plan_partitions(value, disk(32ULL * 1024 * kMiB, "/dev/sdb", 4096));
  CHECK(plan.partitions[0].start_sector == 256);
  CHECK(plan.partitions[0].size_mib == 512);
  CHECK(plan.partitions[1].start_sector % 256 == 0);
  CHECK(plan.partitions[1].start_sector + plan.partitions[1].size_sectors == plan.disk.size_bytes / 4096 - 33);

  auto mismatch = disk(120ULL * 1024 * kMiB);
  mismatch.size_bytes = 40ULL * 1024 * kMiB;
  try {
    (void)plan_partitions(answer(), mismatch);
    CHECK(false);
  } catch (const ValidationError&) {}
}

void test_capacity_and_overflow() {
  auto unconstrained = answer();
  unconstrained.disk_size_bytes.reset();
  for (std::uint64_t mib : {512ULL, 8192ULL, 8700ULL}) {
    try {
      (void)plan_partitions(unconstrained, disk(mib * kMiB));
      CHECK(false);
    } catch (const ValidationError&) {}
  }
  (void)plan_partitions(unconstrained, disk(9ULL * 1024 * kMiB));
  auto too_small_home = answer();
  too_small_home.separate_home = true;
  too_small_home.root_size_mib = 8192;
  too_small_home.root_size_mib = 6000;
  too_small_home.root_size_mib = 22000;
  too_small_home.disk = "/dev/sda";
  try {
    (void)plan_partitions(too_small_home, disk(24ULL * 1024 * kMiB));
    CHECK(false);
  } catch (const ValidationError&) {}

  auto huge = answer();
  huge.disk_size_bytes.reset();
  huge.swapfile_size_mib = std::numeric_limits<std::uint64_t>::max();
  try {
    (void)make_installation_plan(huge, disk(32ULL * 1024 * kMiB));
    CHECK(false);
  } catch (const ValidationError&) {}
}

void test_command_generation() {
  auto value = answer();
  value.disk_size_bytes.reset();
  const auto plan = make_installation_plan(value, disk(80ULL * 1024 * kMiB));
  contains_command(plan, "sfdisk");
  contains_command(plan, "mkfs.vfat");
  contains_command(plan, "mkfs.ext4");
  contains_command(plan, "unsquashfs");
  contains_command(plan, "grub-install");
  contains_command(plan, "--target=x86_64-efi");
  contains_command(plan, "update-grub");
  contains_command(plan, "update-initramfs");
  contains_command(plan, "locale-gen");
  contains_command(plan, "chpasswd");
  contains_command(plan, "UUID=%s");
  CHECK(!has_secret(plan, "not-a-real-test-password"));
  const auto& password = *std::ranges::find_if(plan.steps, [](const InstallStep& step) {
    return step.name == "user: set password over protected stdin";
  });
  CHECK(password.command.standard_input == "iris:not-a-real-test-password\n");
  auto printing = plan;
  (void)printing;
}

void test_bios_btrfs_home_and_swap() {
  auto value = answer(R"({"disk":"/dev/sda","firmware":"bios","filesystem":{"root":"btrfs"},"swapfile_mib":2048,"source":"/filesystem.squashfs","hostname":"host","user":{"name":"user","password":"ioniremosritp斯文-DoNotPrint"}})");
  const auto plan = make_installation_plan(value, disk(64ULL * 1024 * kMiB));
  CHECK(plan.partitions.partitions.size() == 2);
  contains_command(plan, "mkfs.btrfs");
  contains_command(plan, "--target=i386-pc");
  contains_command(plan, "@home");
  contains_command(plan, "@var_log");
  contains_command(plan, "@snapshots");
  contains_command(plan, "@swap");
  contains_command(plan, "chattr +C");
  contains_command(plan, "compression none");
  contains_command(plan, "mkswap -f /swap/swapfile");
  const auto unsquash = std::ranges::find_if(plan.steps, [](const auto& step) { return step.name == "deploy: extract SquashFS image"; });
  const auto home = std::ranges::find_if(plan.steps, [](const auto& step) { return step.name == "mount: Btrfs @home before deployment"; });
  REQUIRE(unsquash != plan.steps.end());
  REQUIRE(home != plan.steps.end());
  CHECK(home < unsquash);
  CHECK(!has_secret(plan, "ioniremosritp斯文-DoNotPrint"));
}

void test_shell_quoted_configuration() {
  auto value = answer(R"({"disk":"/dev/sda","source":"/filesystem.squashfs","hostname":"host","timezone":"Etc/GMT+5","full_name":"unused","user":{"name":"user","full_name":"User's Full Name","password":"password"}})");
  const auto plan = make_installation_plan(value, disk(48ULL * 1024 * kMiB));
  CHECK(shell_escape("plain/path-1.0") == "plain/path-1.0");
  CHECK(shell_escape("") == "''");
  CHECK(shell_escape("a b") == "'a b'");
  CHECK(shell_escape("a'b") == "'a'\\''b'");
  CHECK(shell_escape("$(touch /tmp/pwn)") == "'$(touch /tmp/pwn)'");
  contains_command(plan, "'User'\\''s Full Name'");
  contains_command(plan, "Etc/GMT+5");
}

void test_step_runner_success_cleanup() {
  FakeRunner runner;
  StepRunner steps(runner, [](const std::string&) {});
  const std::vector<InstallStep> plan = {
      InstallStep{"one", Command{{"echo", "one"}}},
      InstallStep{"mount root", Command{{"mount", "one"}}, false, "/mnt/example"},
      InstallStep{"mount boot", Command{{"mount", "two"}}, false, "/mnt/example/boot"},
      InstallStep{"last", Command{{"echo", "last"}}},
  };
  const auto report = steps.run(plan);
  REQUIRE(report.success);
  REQUIRE(report.failed_step == StepRunReport::kNoFailedStep);
  REQUIRE(runner.calls.size() == 6);
  CHECK(runner.calls[4].arguments == std::vector<std::string>({"umount", "-R", "/mnt/example/boot"}));
  CHECK(runner.calls[5].arguments == std::vector<std::string>({"umount", "-R", "/mnt/example"}));
  // A second unmount_all() must not repeat commands after tracking is cleared.
  CHECK(steps.unmount_all());
  CHECK(runner.calls.size() == 6);
}

void test_step_runner_stops_and_rolls_back() {
  FakeRunner runner;
  ProcessResult error;
  error.exit_code = 42;
  error.standard_error = "simulated mkfs failure";
  runner.responses.push_back({});
  runner.responses.push_back({});
  runner.responses.push_back(error);
  StepRunner steps(runner, [](const std::string&) {});
  const std::vector<InstallStep> plan = {
      InstallStep{"mount root", Command{{"mount", "one"}}, false, "/mnt/example"},
      InstallStep{"mount boot", Command{{"mount", "two"}}, false, "/mnt/example/boot"},
      InstallStep{"format", Command{{"mkfs.ext4", "-F", "x"}}},
      InstallStep{"must not run", Command{{"chroot", "x"}}},
  };
  const auto report = steps.run(plan);
  CHECK(!report.success);
  CHECK(report.failed_step == 2);
  CHECK(report.error.find("simulated mkfs failure") != std::string::npos);
  REQUIRE(runner.calls.size() == 5);
  CHECK(runner.calls[3].arguments == std::vector<std::string>({"umount", "-R", "/mnt/example/boot"}));
  CHECK(runner.calls[4].arguments == std::vector<std::string>({"umount", "-R", "/mnt/example"}));
}

void test_explicit_unmount_then_cleanup() {
  FakeRunner runner;
  StepRunner steps(runner, [](const std::string&) {});
  auto report = steps.run({
      InstallStep{"mount", Command{{"mount", "/mnt/example"}}, false, "/mnt/example"},
      InstallStep{"unmount", Command{{"umount", "/mnt/example"}}, false, {}, "/mnt/example"},
      InstallStep{"last", Command{{"true"}}},
  });
  REQUIRE(report.success);
  REQUIRE(runner.calls.size() == 3);
}

void test_lazy_unmount_fallback_and_failure() {
  FakeRunner runner;
  ProcessResult failed;
  failed.exit_code = 32;
  failed.standard_error = "busy";
  runner.responses.push_back({});
  runner.responses.push_back(failed);
  runner.responses.push_back({});
  StepRunner steps(runner, [](const std::string&) {});
  const auto report = steps.run({InstallStep{"mount", Command{{"mount", "x"}}, false, "/mnt/example"}});
  CHECK(report.success);
  REQUIRE(runner.calls.size() == 3);
  CHECK(runner.calls[2].arguments == std::vector<std::string>({"umount", "-R", "-l", "/mnt/example"}));

  FakeRunner all_fail;
  all_fail.responses.push_back({});
  all_fail.responses.push_back(failed);
  all_fail.responses.push_back(failed);
  StepRunner all_fail_steps(all_fail, [](const std::string&) {});
  const auto failed_report = all_fail_steps.run({InstallStep{"mount", Command{{"mount", "x"}}, false, "/mnt/example"}});
  CHECK(!failed_report.success);
  CHECK(!failed_report.rollback_succeeded);
  REQUIRE(all_fail.calls.size() == 3);
}

void test_gates_and_sensitive_redaction() {
  FakeRunner runner;
  auto value = answer();
  auto report = execute_installation(value, true, runner);
  CHECK(!report.success);
  CHECK(report.error.find("destructive:true") != std::string::npos);
  CHECK(runner.calls.empty());
  value.destructive = true;
  report = execute_installation(value, false, runner);
  CHECK(!report.success);
  CHECK(report.error.find("--i-understand-this-erases-data") != std::string::npos);
  CHECK(runner.calls.empty());
  value.disk_size_bytes.reset();
  const auto plan = make_installation_plan(value, disk(64ULL * 1024 * kMiB));
  const auto displayed = json::stringify(installation_plan_to_json(plan));
  CHECK(displayed.find("\"dry_run\": true") != std::string::npos);
  CHECK(displayed.find("not-a-real-test-password") == std::string::npos);

  FakeRunner fake;
  ProcessResult sensitive;
  sensitive.exit_code = 1;
  sensitive.standard_output = "secret-stdout";
  sensitive.standard_error = "secret-stderr";
  fake.responses.push_back(sensitive);
  std::string logs;
  StepRunner steps(fake, [&](const std::string& line) { logs += line + "\n"; });
  Command command{{"sh", "-c", "false"}};
  command.sensitive_input = true;
  command.standard_input = "secret-input";
  const auto sensitive_report = steps.run({InstallStep{"password", command}});
  CHECK(!sensitive_report.success);
  CHECK(logs.find("secret-stdout") == std::string::npos);
  CHECK(logs.find("secret-stderr") == std::string::npos);
  CHECK(logs.find("secret-input") == std::string::npos);
}

void test_real_process_io() {
  SystemProcessRunner runner;
  auto success = runner.run(Command{{"printf", "hello-a b"}});
  CHECK(success.succeeded());
  CHECK(success.standard_output == "hello-a b");
  auto stdout_and_stderr = runner.run(Command{{"sh", "-c", "cat; printf err >&2; exit 7"}, "opaque a'b\n"});
  CHECK(stdout_and_stderr.exit_code == 7);
  CHECK(!stdout_and_stderr.succeeded());
  CHECK(stdout_and_stderr.standard_output == "opaque a'b\n");
  CHECK(stdout_and_stderr.standard_error == "err");
  auto missing = runner.run(Command{{"matinstaller-definitely-not-a-real-binary"}});
  CHECK(!missing.succeeded());
  CHECK(!missing.error.empty());
  auto capped = runner.run(Command{{"sh", "-c", "printf '%0100d' 0"}, "", false, 16});
  CHECK(capped.succeeded());
  CHECK(capped.output_truncated);
  CHECK(capped.standard_output.size() <= 16);
}

}  // namespace

int main() {
  const std::vector<std::pair<const char*, std::function<void()>>> tests = {
      {"json_parser", test_json_parser},
      {"answer_validation", test_answer_validation},
      {"uefi_geometry", test_uefi_geometry},
      {"bios_geometry", test_bios_geometry},
      {"separate_home", test_separate_home},
      {"4k_alignment", test_4k_alignment},
      {"capacity_and_overflow", test_capacity_and_overflow},
      {"command_generation", test_command_generation},
      {"bios_btrfs_home_and_swap", test_bios_btrfs_home_and_swap},
      {"shell_quoted_configuration", test_shell_quoted_configuration},
      {"step_runner_success_cleanup", test_step_runner_success_cleanup},
      {"step_runner_stops_and_rolls_back", test_step_runner_stops_and_rolls_back},
      {"explicit_unmount_then_cleanup", test_explicit_unmount_then_cleanup},
      {"lazy_unmount_fallback_and_failure", test_lazy_unmount_fallback_and_failure},
      {"gates_and_sensitive_redaction", test_gates_and_sensitive_redaction},
      {"real_process_io", test_real_process_io},
  };
  for (const auto& [name, test] : tests) {
    try {
      test();
      std::cout << "PASS " << name << '\n';
    } catch (const std::exception& exception) {
      std::cerr << "FAIL " << name << ": " << exception.what() << '\n';
      ++failures;
    }
  }
  std::cout << tests.size() << " tests; " << failures << " failures\n";
  return failures == 0 ? 0 : 1;
}
