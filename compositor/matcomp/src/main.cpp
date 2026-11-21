#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include "matcomp/config.hpp"
#include "matcomp/server.hpp"

void usage(const char* program) {
    std::cerr << "Usage: " << program << " [--config FILE] [--headless] [--self-test] [command]\n";
}

int main(int argc, char** argv) {
    // Diagnostics are piped to the serial console in the live session; keep
    // them line-buffered so a stall is visible immediately.
    setvbuf(stdout, nullptr, _IOLBF, 0);
    matcomp::Config configuration;
    std::filesystem::path config_path;
    bool self_test = false;
    std::string command;
    if (const char* path = std::getenv("MAT_COMPOSITOR_CONFIG")) config_path = path;
    else if (const char* home = std::getenv("HOME")) config_path = std::filesystem::path(home) / ".config/materialized/matcomp.toml";
    if (std::filesystem::exists(config_path)) matcomp::load_default_config(configuration);
    for (int i = 1; i < argc; ++i) {
        const std::string_view argument = argv[i];
        if (argument == "--headless") {
            configuration.backend = matcomp::BackendPreference::Headless;
            setenv("WLR_BACKENDS", "headless", 1);
        } else if (argument == "--config" && i + 1 < argc) {
            configuration = {};
            matcomp::load_config_file(argv[++i], configuration);
        } else if (argument == "--self-test") {
            self_test = true;
        } else if (argument == "--help") {
            usage(argv[0]);
            return 0;
        } else if (!argument.starts_with("-")) {
            command = argv[i];
        } else {
            usage(argv[0]);
            return 64;
        }
    }
    try {
        matcomp::Server server(configuration);
        server.initialize();
        if (self_test) return server.self_test() ? 0 : 1;
        server.run(command);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "matcomp: " << error.what() << '\n';
        return 1;
    }
}
