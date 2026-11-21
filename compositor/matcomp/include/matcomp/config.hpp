#pragma once

#include <filesystem>
#include <iosfwd>
#include <string>

namespace matcomp {

enum class BackendPreference { Auto, Headless, Drm };

struct Config {
    BackendPreference backend{BackendPreference::Auto};
    int headless_width{1280};
    int headless_height{720};
    // "auto" selects the monitor's preferred mode. Otherwise WIDTHxHEIGHT[@HZ].
    std::string output_mode{"auto"};
    int window_gap{8};
    std::string xkb_layout;
    std::string xkb_model;
    std::string xkb_variant;
    std::string xkb_options;
    int repeat_rate{25};
    int repeat_delay{600};
    bool focus_follows_mouse{false};
    bool server_decorations{true};
};

// Parses the small, deliberately documented TOML subset used by matcomp.
// Existing values in config are retained when a key is absent, which permits
// system/user configuration layering. Errors include the source line number.
void load_config_file(const std::filesystem::path& path, Config& config);
void load_default_config(Config& config);

BackendPreference parse_backend(const std::string& value);
std::string_view to_string(BackendPreference backend);
void print_config(const Config& config, std::ostream& stream);

} // namespace matcomp
