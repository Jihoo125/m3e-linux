#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace matinstall {

struct Command {
  Command(std::vector<std::string> argv = {}, std::string stdin_text = {},
          bool sensitive = false, std::size_t output_bytes = 1024 * 1024)
      : arguments(std::move(argv)), standard_input(std::move(stdin_text)),
        sensitive_input(sensitive), output_limit(output_bytes) {}

  std::vector<std::string> arguments;
  std::string standard_input;
  bool sensitive_input = false;
  std::size_t output_limit = 1024 * 1024;
};

struct ProcessResult {
  int exit_code = 0;
  int terminating_signal = 0;
  std::string standard_output;
  std::string standard_error;
  std::string error;
  bool output_truncated = false;

  bool succeeded() const { return exit_code == 0 && terminating_signal == 0 && error.empty(); }
};

class ProcessRunner {
 public:
  virtual ~ProcessRunner() = default;
  virtual ProcessResult run(const Command& command) = 0;
};

// POSIX implementation: resolves an executable below a fixed trusted PATH and
// uses fork/execve with explicit argv/environment. Concurrent stdin/stdout/
// stderr pumping, bounded captured output and waitpid are provided.
// No shell is used for ordinary commands. Explicit target configuration steps
// use sh -c only with shell-escaped, prevalidated interpolated values.
bool host_tool_exists(const std::string& name);
std::string resolve_executable(const std::string& name);

class SystemProcessRunner final : public ProcessRunner {
 public:
  ProcessResult run(const Command& command) override;
};

// Used only to render auditable command logs. It is not an execution API.
std::string shell_escape(std::string_view argument);
std::string format_command(const Command& command);

struct InstallStep {
  InstallStep(std::string step_name, Command step_command, bool is_destructive = false,
              std::string mount_to_register = {}, std::string mount_to_unregister = {})
      : name(std::move(step_name)), command(std::move(step_command)), destructive(is_destructive),
        registers_mount(std::move(mount_to_register)), unregisters_mount(std::move(mount_to_unregister)) {}

  std::string name;
  Command command;
  bool destructive = false;
  std::string registers_mount;
  std::string unregisters_mount;
};

struct StepRunReport {
  static constexpr std::size_t kNoFailedStep = static_cast<std::size_t>(-1);
  bool success = false;
  std::size_t failed_step = kNoFailedStep;
  std::string error;
  bool rollback_succeeded = true;
};

using LogSink = std::function<void(const std::string&)>;

class StepRunner {
 public:
  StepRunner(ProcessRunner& runner, LogSink sink);
  ~StepRunner();
  StepRunner(const StepRunner&) = delete;
  StepRunner& operator=(const StepRunner&) = delete;

  // Stops at the first failure. Successful mounts are recursively unmounted in
  // reverse order on both failure and successful completion. Lazy unmount is a
  // logged last resort; a failed cleanup makes the entire report unsuccessful.
  StepRunReport run(const std::vector<InstallStep>& steps);
  bool unmount_all();

 private:
  ProcessRunner& runner_;
  LogSink sink_;
  std::vector<std::string> mounted_;
};

}  // namespace matinstall
