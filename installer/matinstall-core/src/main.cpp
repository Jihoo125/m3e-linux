#include "matinstall/answer.hpp"
#include "matinstall/installer.hpp"
#include "matinstall/json.hpp"
#include "matinstall/planner.hpp"
#include "matinstall/process.hpp"

#include <iostream>
#include <optional>
#include <string>
#include <string_view>

namespace {

void usage(std::ostream& output) {
  output << "matinstall-core — Materialized Linux installation engine\n\n"
         << "Usage:\n"
         << "  matinstall-core probe\n"
         << "  matinstall-core plan ANSWER.json\n"
         << "  matinstall-core validate ANSWER.json\n"
         << "  matinstall-core run ANSWER.json --i-understand-this-erases-data\n\n"
         << "plan is always a dry run. run performs whole-disk destructive operations;\n"
         << "it requires destructive:true in the answer and the explicit flag.\n"
         << "probe and run require Linux util-linux tools; run additionally requires\n"
         << "root, a real whole disk, a SquashFS image and target installation tools.\n";
}

std::string json_message(std::string_view type, const std::string& message) {
  matinstall::json::Value result = matinstall::json::Value::object();
  result.set(std::string(type), message);
  return matinstall::json::stringify(result, 0);
}

int run(int argc, char** argv) {
  if (argc == 2 && (std::string_view(argv[1]) == "--help" || std::string_view(argv[1]) == "-h")) {
    usage(std::cout);
    return 0;
  }
  if (argc < 2) {
    usage(std::cerr);
    return 64;
  }
  const std::string command = argv[1];
  matinstall::SystemProcessRunner runner;
  if (command == "probe") {
    if (argc != 2) {
      std::cerr << "probe takes no arguments\n";
      return 64;
    }
    std::cout << matinstall::probe_lsblk(runner);
    return 0;
  }
  if (command != "validate" && command != "plan" && command != "run") {
    std::cerr << "unknown command: " << command << "\n";
    usage(std::cerr);
    return 64;
  }
  if (argc < 3) {
    std::cerr << command << " requires an answer JSON file\n";
    return 64;
  }
  const std::string answer_path = argv[2];
  const bool confirmation = argc == 4 && std::string_view(argv[3]) == "--i-understand-this-erases-data";
  if ((command == "run" && !(argc == 3 || confirmation)) ||
      (command != "run" && argc != 3)) {
    std::cerr << "usage: matinstall-core " << command << " ANSWER.json"
              << (command == "run" ? " --i-understand-this-erases-data" : "") << "\n";
    return 64;
  }
  const matinstall::Answer answer = matinstall::load_answer(answer_path);
  if (command == "validate") {
    matinstall::Disk disk;
    disk.path = answer.disk;
    disk.probed = false;
    if (answer.disk_size_bytes.has_value()) {
      disk.size_bytes = answer.disk_size_bytes.value();
      disk.logical_sector_size = answer.logical_sector_size.value_or(512);
    } else {
      // Structural validation alone cannot establish that a disk is large
      // enough, because no physical I/O is allowed by this command.
      std::cout << "{\"valid\":true,\"disk_size_validated\":false}\n";
      return 0;
    }
    const auto plan = matinstall::plan_partitions(answer, disk);
    std::cout << "{\"valid\":true,\"disk_size_validated\":true,\"partitions\":"
              << plan.partitions.size() << "}\n";
    return 0;
  }
  if (command == "plan") {
    matinstall::Disk disk;
    disk.path = answer.disk;
    if (answer.disk_size_bytes.has_value()) {
      disk.size_bytes = answer.disk_size_bytes.value();
      disk.logical_sector_size = answer.logical_sector_size.value_or(512);
      disk.probed = false;
    } else {
      // A plan can be safely previewed without the destructive tools, but when
      // a size is omitted on Linux we use read-only lsblk to show real geometry.
      disk = matinstall::inspect_disk(answer.disk, runner);
      if (disk.read_only || disk.in_use) {
        throw matinstall::ValidationError("selected disk is read-only, mounted or has holders; refusing to plan destructive steps");
      }
    }
    const matinstall::InstallationPlan plan = matinstall::make_installation_plan(answer, disk);
    std::cout << matinstall::json::stringify(matinstall::installation_plan_to_json(plan));
    return 0;
  }
  const matinstall::ExecutionReport report = matinstall::execute_installation(answer, confirmation, runner);
  if (!report.success) {
    std::cerr << json_message("error", report.error);
    return 1;
  }
  matinstall::json::Value output = matinstall::json::Value::object();
  output.set("success", true);
  output.set("log_file", report.log_file);
  output.set("unmounted", true);
  std::cout << matinstall::json::stringify(output);
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    return run(argc, argv);
  } catch (const matinstall::ValidationError& error) {
    std::cerr << json_message("error", error.what());
    return 2;
  } catch (const std::exception& error) {
    std::cerr << json_message("error", error.what());
    return 2;
  }
}
