#include "matinstall/process.hpp"
#include <cstdio>

#include <algorithm>
#include <array>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <filesystem>
#include <sstream>
#include <utility>

namespace matinstall {
namespace {

struct Pipe {
  int read = -1;
  int write = -1;
};

void close_fd(int& fd) {
  if (fd >= 0) {
    // Avoid an unbounded EINTR retry loop: Linux closes the descriptor even
    // when close reports an interruption. Other descriptors are managed by RAII
    // in this function, so process-failure cleanup is still deterministic.
    while (::close(fd) != 0 && errno == EINTR) {
      if (::fcntl(fd, F_GETFD) != -1 || errno != EBADF) continue;
      break;
    }
    fd = -1;
  }
}

bool make_pipe(Pipe& pipe) {
  int fds[2];
  if (::pipe2(fds, O_CLOEXEC) != 0) return false;
  pipe.read = fds[0];
  pipe.write = fds[1];
  return true;
}

bool close_on_exec(int fd) { return ::fcntl(fd, F_SETFD, FD_CLOEXEC) == 0; }

void append_errno(std::string& output, std::string_view action) {
  output += action;
  output += ": ";
  output += std::strerror(errno);
  output.push_back('\n');
}

void capture(std::string& destination, const char* data, std::size_t size,
             std::size_t limit, bool& truncated) {
  const std::size_t available = destination.size() < limit ? limit - destination.size() : 0;
  const std::size_t keep = std::min(size, available);
  destination.append(data, keep);
  if (keep != size) truncated = true;
}

}  // namespace

std::string shell_escape(std::string_view argument) {
  if (argument.empty()) return "''";
  const bool safe = std::ranges::all_of(argument, [](char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
           c == '_' || c == '-' || c == '.' || c == '/' || c == ':' || c == '@' || c == '+' || c == ',';
  });
  if (safe) return std::string(argument);
  std::string result = "'";
  for (const char c : argument) {
    if (c == '\'') result += "'\\''";
    else result.push_back(c);
  }
  result.push_back('\'');
  return result;
}

std::string format_command(const Command& command) {
  std::string result;
  for (std::size_t i = 0; i < command.arguments.size(); ++i) {
    if (i != 0) result.push_back(' ');
    result += shell_escape(command.arguments[i]);
  }
  if (!command.standard_input.empty()) result += "  # stdin: <redacted>";
  return result;
}

std::string resolve_executable(const std::string& name) {
  if (name.empty() || name.find('\0') != std::string::npos || name.find("..") != std::string::npos) return {};
  if (name.front() == '/') {
    if (::access(name.c_str(), X_OK) != 0) return {};
    struct stat info {};
    return ::stat(name.c_str(), &info) == 0 && S_ISREG(info.st_mode) ? name : "";
  }
  for (const char* directory : {"/usr/sbin", "/usr/bin", "/sbin", "/bin"}) {
    const std::string candidate = std::string(directory) + "/" + name;
    if (::access(candidate.c_str(), X_OK) != 0) continue;
    struct stat info {};
    if (::stat(candidate.c_str(), &info) == 0 && S_ISREG(info.st_mode)) return candidate;
  }
  return {};
}

bool host_tool_exists(const std::string& name) { return !resolve_executable(name).empty(); }

ProcessResult SystemProcessRunner::run(const Command& command) {
  ProcessResult result;
  if (command.arguments.empty() || command.arguments.front().empty()) {
    result.error = "cannot execute an empty argv";
    return result;
  }
  if (std::ranges::any_of(command.arguments, [](const std::string& arg) { return arg.find('\0') != std::string::npos; })) {
    result.error = "argv contains an embedded NUL";
    return result;
  }
  const std::string executable = resolve_executable(command.arguments.front());
  if (executable.empty()) {
    // A trusted name that does not exist is a spawn failure, distinct from an
    // empty argv or NUL-containing input. Callers/tests can distinguish it from
    // an application exiting with its own code.
    result.exit_code = 126;
    if (!command.sensitive_input) result.standard_error = "exec " + executable + ": No such file or directory";
    result.error = "trusted executable not found or not executable: " + command.arguments.front();
    return result;
  }
  // Resolve before fork. In the child use only fixed-size stack data and
  // async-signal-safe operations; no heap allocation or exception can escape
  // the multi-thread-hostile post-fork window.
  std::vector<char*> argv(command.arguments.size() + 1, nullptr);
  for (std::size_t i = 0; i < command.arguments.size(); ++i) argv[i] = const_cast<char*>(command.arguments[i].c_str());
  Pipe input, output, error;
  if (!make_pipe(input) || !make_pipe(output) || !make_pipe(error)) {
    result.error = std::string("pipe2 failed: ") + std::strerror(errno);
    close_fd(input.read); close_fd(input.write);
    close_fd(output.read); close_fd(output.write);
    close_fd(error.read); close_fd(error.write);
    return result;
  }

  const pid_t pid = ::fork();
  if (pid < 0) {
    result.error = std::string("fork failed: ") + std::strerror(errno);
    close_fd(input.read); close_fd(input.write);
    close_fd(output.read); close_fd(output.write);
    close_fd(error.read); close_fd(error.write);
    return result;
  }
  if (pid == 0) {
    if (::dup2(input.read, STDIN_FILENO) < 0 || ::dup2(output.write, STDOUT_FILENO) < 0 ||
        ::dup2(error.write, STDERR_FILENO) < 0) {
      constexpr char message[] = "matinstall-core: dup2 failed\n";
      const ssize_t written = ::write(STDERR_FILENO, message, sizeof(message) - 1);
      (void)written;
      _exit(126);
    }
    close_fd(input.read); close_fd(input.write);
    close_fd(output.read); close_fd(output.write);
    close_fd(error.read); close_fd(error.write);
    char environment_path[] = "PATH=/usr/sbin:/usr/bin:/sbin:/bin";
    char environment_locale[] = "LC_ALL=C";
    char* environment[] = {environment_path, environment_locale, nullptr};
    ::execve(executable.c_str(), argv.data(), environment);
    constexpr char message[] = "matinstall-core: execve failed\n";
    const ssize_t written = ::write(STDERR_FILENO, message, sizeof(message) - 1);
    (void)written;
    _exit(126);
  }

  close_fd(input.read);
  close_fd(output.write);
  close_fd(error.write);
  std::size_t input_offset = 0;
  std::array<char, 16384> buffer{};
  struct Channel {
    int* fd;
    short events;
    std::string* capture_output;
    std::size_t limit;
    bool* truncated;
  };
  std::array<Channel, 3> channels{{
      {&input.write, static_cast<short>(input_offset < command.standard_input.size() ? POLLOUT : 0), nullptr, 0, nullptr},
      {&output.read, POLLIN, &result.standard_output, command.output_limit, &result.output_truncated},
      {&error.read, POLLIN, &result.standard_error, command.output_limit, &result.output_truncated},
  }};
  if (command.standard_input.empty()) close_fd(input.write);
  else close_on_exec(input.write);
  close_on_exec(output.read);
  close_on_exec(error.read);

  bool active = true;
  while (active) {
    active = false;
    std::array<pollfd, 3> fds{};
    for (std::size_t index = 0; index < channels.size(); ++index) {
      Channel& channel = channels[index];
      fds[index].fd = *channel.fd;
      fds[index].events = *channel.fd >= 0 ? channel.events : 0;
      if (*channel.fd >= 0) active = true;
    }
    if (!active) break;
    int poll_result;
    do {
      poll_result = ::poll(fds.data(), fds.size(), -1);
    } while (poll_result < 0 && errno == EINTR);
    if (poll_result < 0) {
      result.error = std::string("poll failed: ") + std::strerror(errno);
      break;
    }
    for (std::size_t index = 0; index < channels.size(); ++index) {
      Channel& channel = channels[index];
      if (*channel.fd < 0 || fds[index].revents == 0) continue;
      if (index == 0) {
        if ((fds[index].revents & POLLOUT) != 0) {
          const ssize_t written = ::write(*channel.fd, command.standard_input.data() + input_offset,
                                          command.standard_input.size() - input_offset);
          if (written >= 0) {
            input_offset += static_cast<std::size_t>(written);
            if (input_offset == command.standard_input.size()) close_fd(*channel.fd);
          } else if (errno != EINTR && errno != EAGAIN) {
            if (errno != EPIPE) append_errno(result.standard_error, "write(child stdin)");
            close_fd(*channel.fd);
          }
        } else {
          // No reader or an error: do not spin or block forever.
          close_fd(*channel.fd);
        }
      } else {
        const ssize_t count = ::read(*channel.fd, buffer.data(), buffer.size());
        if (count > 0) {
          capture(*channel.capture_output, buffer.data(), static_cast<std::size_t>(count),
                  channel.limit, *channel.truncated);
        } else if (count == 0 || (errno != EINTR && errno != EAGAIN)) {
          close_fd(*channel.fd);
        }
      }
    }
  }
  close_fd(input.write); close_fd(output.read); close_fd(error.read);
  int status = 0;
  pid_t waited;
  do {
    waited = ::waitpid(pid, &status, 0);
  } while (waited < 0 && errno == EINTR);
  if (waited < 0) {
    if (!result.error.empty()) result.error += "; ";
    result.error += std::string("waitpid failed: ") + std::strerror(errno);
  } else if (WIFEXITED(status)) {
    result.exit_code = WEXITSTATUS(status);
  } else if (WIFSIGNALED(status)) {
    result.terminating_signal = WTERMSIG(status);
    result.exit_code = 128 + result.terminating_signal;
  } else {
    result.error = "child ended in an unexpected wait state";
  }
  return result;
}

StepRunner::StepRunner(ProcessRunner& runner, LogSink sink) : runner_(runner), sink_(std::move(sink)) {}

StepRunner::~StepRunner() {
  if (!mounted_.empty()) (void)unmount_all();
}


StepRunReport StepRunner::run(const std::vector<InstallStep>& steps) {
  StepRunReport report;
  for (std::size_t index = 0; index < steps.size(); ++index) {
    const InstallStep& step = steps[index];
    sink_("START step=" + std::to_string(index + 1) + " name=" + step.name +
          " command=" + format_command(step.command));
    if (!step.registers_mount.empty() &&
        std::ranges::find(mounted_, step.registers_mount) != mounted_.end()) {
      report.failed_step = index;
      report.error = "step tried to register an already-tracked mount: " + step.registers_mount;
      break;
    }
    const ProcessResult result = runner_.run(step.command);
    if (!result.succeeded()) {
      report.failed_step = index;
      std::ostringstream error;
      error << "step '" << step.name << "' failed";
      if (result.terminating_signal != 0) error << "; signal=" << result.terminating_signal;
      else error << "; exit=" << result.exit_code;
      if (!result.error.empty()) {
        error << (step.command.sensitive_input ? ": process error redacted" : ": " + result.error);
      }
      if (!step.command.sensitive_input) {
        if (!result.standard_error.empty()) error << "; stderr:\n" << result.standard_error;
        if (!result.standard_output.empty()) error << "; stdout:\n" << result.standard_output;
      } else {
        error << "; sensitive command output redacted";
      }
      report.error = error.str();
      sink_("ERROR step=" + step.name + " " + report.error);
      break;
    }
    if (!step.command.sensitive_input && !result.standard_error.empty()) {
      sink_("STDERR step=" + step.name + "\n" + result.standard_error);
    } else if (step.command.sensitive_input && (!result.standard_error.empty() || !result.standard_output.empty())) {
      sink_("REDACTED output from sensitive step=" + step.name);
    }
    if (!step.registers_mount.empty()) mounted_.push_back(step.registers_mount);
    if (!step.unregisters_mount.empty()) {
      const auto found = std::ranges::find(mounted_, step.unregisters_mount);
      if (found == mounted_.end()) {
        report.failed_step = index;
        report.error = "step unregistered an unknown mount: " + step.unregisters_mount;
        break;
      }
      mounted_.erase(found);
    }
    sink_("OK step=" + std::to_string(index + 1) + " name=" + step.name +
            (!result.standard_output.empty() && !step.command.sensitive_input ? "\n" + result.standard_output : ""));
  }
  if (report.failed_step == StepRunReport::kNoFailedStep) report.success = true;
  if (!unmount_all()) {
    report.success = false;
    report.rollback_succeeded = false;
    if (report.error.empty()) report.error = "install completed a step but rollback unmount failed";
    else report.error += "; rollback unmount failed";
  }
  return report;
}

bool StepRunner::unmount_all() {
  bool success = true;
  while (!mounted_.empty()) {
    const std::string mount = mounted_.back();
    Command command{{"umount", "-R", mount}};
    const ProcessResult result = runner_.run(command);
    if (result.succeeded()) {
      sink_("ROLLBACK umount -R " + shell_escape(mount));
      mounted_.pop_back();
      continue;
    }
    sink_("ERROR rollback-unmount=" + mount + " stderr=" + result.standard_error);
    const ProcessResult lazy = runner_.run(Command{{"umount", "-R", "-l", mount}});
    if (lazy.succeeded()) {
      sink_("WARNING rollback-lazy-unmount=" + mount);
      mounted_.pop_back();
    } else {
      sink_("ERROR rollback-lazy-unmount=" + mount + " stderr=" + lazy.standard_error);
      // Keep the entry tracked in case a future caller retries cleanup.
      return false;
    }
  }
  return success;
}

}  // namespace matinstall
