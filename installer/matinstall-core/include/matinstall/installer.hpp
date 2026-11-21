#pragma once

#include "matinstall/json.hpp"
#include "matinstall/planner.hpp"
#include "matinstall/process.hpp"

#include <string>

namespace matinstall {

inline constexpr const char* kTargetRoot = "/mnt/materialized";

struct InstallationPlan {
  Answer answer;
  Disk disk;
  PartitionPlan partitions;
  std::vector<InstallStep> steps;
  std::vector<std::string> warnings;
};

// Pure command generation after geometry has been supplied. Password input is
// carried in memory but is always redacted by installation_plan_to_json().
InstallationPlan make_installation_plan(const Answer& answer, const Disk& disk);
json::Value installation_plan_to_json(const InstallationPlan& plan);

std::string probe_lsblk(ProcessRunner& runner);
Disk inspect_disk(const std::string& disk_path, ProcessRunner& runner);

struct ExecutionReport {
  bool success = false;
  std::string log_file;
  std::string error;
};

// Both confirmation_flag and answer.destructive must be true. This function
// also requires euid 0, a real unmounted whole disk, a readable SquashFS and a
// complete tool preflight before its first destructive step.
ExecutionReport execute_installation(const Answer& answer,
                                     bool confirmation_flag,
                                     ProcessRunner& runner,
                                     const std::string& requested_log_file = {});

}  // namespace matinstall
