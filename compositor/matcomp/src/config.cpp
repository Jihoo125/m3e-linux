#include "matcomp/config.hpp"

#include <charconv>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <limits>
#include <optional>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string_view>

namespace matcomp {
namespace {

std::string trim(std::string_view value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string_view::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t\r\n");
    return std::string(value.substr(first, last - first + 1));
}

[[noreturn]] void fail(const std::filesystem::path& path, std::size_t line,
        const std::string& message) {
    throw std::runtime_error(path.string() + ":" + std::to_string(line) +
        ": " + message);
}

std::pair<std::string, std::string> key_value(const std::string& line,
        const std::filesystem::path& path, std::size_t line_number) {
    bool quoted = false;
    char quote = 0;
    for (std::size_t i = 0; i < line.size(); ++i) {
        const char c = line[i];
        if (quoted) {
            if (quote == '"' && c == '\\') {
                ++i;
            } else if (c == quote) {
                quoted = false;
            }
        } else if (c == '"' || c == '\'') {
            quoted = true;
            quote = c;
        } else if (c == '=') {
            auto key = trim(std::string_view(line).substr(0, i));
            if (key.empty()) {
                fail(path, line_number, "empty key");
            }
            return {key, trim(std::string_view(line).substr(i + 1))};
        }
    }
    fail(path, line_number, "expected key = value");
}

std::string parse_string(const std::string& value,
        const std::filesystem::path& path, std::size_t line_number) {
    if (value.size() < 2 || (value.front() != '"' && value.front() != '\'') ||
            value.back() != value.front()) {
        fail(path, line_number, "expected a quoted string");
    }
    std::string result;
    for (std::size_t i = 1; i + 1 < value.size(); ++i) {
        char c = value[i];
        if (c == '\\' && value.front() == '"') {
            if (++i + 1 > value.size()) {
                fail(path, line_number, "unfinished string escape");
            }
            c = value[i];
            switch (c) {
            case 'n': result.push_back('\n'); break;
            case 'r': result.push_back('\r'); break;
            case 't': result.push_back('\t'); break;
            case '\\': result.push_back('\\'); break;
            case '"': result.push_back('"'); break;
            default: fail(path, line_number, "unsupported string escape");
            }
        } else {
            result.push_back(c);
        }
    }
    return result;
}

int parse_integer(const std::string& value, const std::filesystem::path& path,
        std::size_t line_number) {
    int parsed = 0;
    const char* begin = value.data();
    const char* end = begin + value.size();
    const auto result = std::from_chars(begin, end, parsed);
    if (result.ec != std::errc{} || result.ptr != end) {
        fail(path, line_number, "expected a 32-bit integer");
    }
    return parsed;
}

bool parse_bool(const std::string& value, const std::filesystem::path& path,
        std::size_t line_number) {
    if (value == "true") {
        return true;
    }
    if (value == "false") {
        return false;
    }
    fail(path, line_number, "expected true or false");
}

void check_positive(int value, const std::filesystem::path& path,
        std::size_t line, const char* key) {
    if (value <= 0) {
        fail(path, line, std::string(key) + " must be positive");
    }
}

void apply(const std::string& section, const std::string& key,
        const std::string& value, Config& config,
        const std::filesystem::path& path, std::size_t line) {
    if (section == "compositor" && key == "backend") {
        config.backend = parse_backend(parse_string(value, path, line));
    } else if (section == "output" && key == "width") {
        config.headless_width = parse_integer(value, path, line);
        check_positive(config.headless_width, path, line, "width");
    } else if (section == "output" && key == "height") {
        config.headless_height = parse_integer(value, path, line);
        check_positive(config.headless_height, path, line, "height");
    } else if (section == "output" && key == "mode") {
        config.output_mode = parse_string(value, path, line);
        if (config.output_mode != "auto") {
            const auto x = config.output_mode.find('x');
            const auto at = config.output_mode.find('@', x);
            if (x == std::string::npos || x == 0) {
                fail(path, line, "mode must be auto or WIDTHxHEIGHT[@HZ]");
            }
            const auto width = config.output_mode.substr(0, x);
            const auto height = config.output_mode.substr(x + 1,
                at == std::string::npos ? at : at - x - 1);
            check_positive(parse_integer(width, path, line), path, line, "mode width");
            check_positive(parse_integer(height, path, line), path, line, "mode height");
            if (at != std::string::npos) {
                if (at + 1 == config.output_mode.size()) {
                    fail(path, line, "missing mode refresh rate");
                }
                std::size_t used = 0;
                const double hz = std::stod(config.output_mode.substr(at + 1), &used);
                if (used != config.output_mode.size() - at - 1 || !std::isfinite(hz) ||
                        hz <= 0.0 || hz > 1000.0) {
                    fail(path, line, "invalid mode refresh rate");
                }
            }
        }
    } else if (section == "window" && key == "gap") {
        config.window_gap = parse_integer(value, path, line);
        if (config.window_gap < 0 || config.window_gap > 256) {
            fail(path, line, "gap must be between 0 and 256");
        }
    } else if (section == "input" && key == "xkb_layout") {
        config.xkb_layout = parse_string(value, path, line);
    } else if (section == "input" && key == "xkb_model") {
        config.xkb_model = parse_string(value, path, line);
    } else if (section == "input" && key == "xkb_variant") {
        config.xkb_variant = parse_string(value, path, line);
    } else if (section == "input" && key == "xkb_options") {
        config.xkb_options = parse_string(value, path, line);
    } else if (section == "input" && key == "repeat_rate") {
        config.repeat_rate = parse_integer(value, path, line);
        if (config.repeat_rate < 0 || config.repeat_rate > 1000) {
            fail(path, line, "repeat_rate must be between 0 and 1000");
        }
    } else if (section == "input" && key == "repeat_delay") {
        config.repeat_delay = parse_integer(value, path, line);
        if (config.repeat_delay < 0 || config.repeat_delay > 10000) {
            fail(path, line, "repeat_delay must be between 0 and 10000");
        }
    } else if (section == "input" && key == "focus_follows_mouse") {
        config.focus_follows_mouse = parse_bool(value, path, line);
    } else {
        fail(path, line, "unknown configuration key [" + section + "] " + key);
    }
}

void load_if_present(const std::filesystem::path& path, Config& config) {
    std::error_code error;
    if (std::filesystem::is_regular_file(path, error)) {
        load_config_file(path, config);
    }
}

} // namespace

BackendPreference parse_backend(const std::string& value) {
    if (value == "auto") {
        return BackendPreference::Auto;
    }
    if (value == "headless") {
        return BackendPreference::Headless;
    }
    if (value == "drm") {
        return BackendPreference::Drm;
    }
    throw std::runtime_error("backend must be auto, headless, or drm (got '" + value + "')");
}

std::string_view to_string(BackendPreference backend) {
    switch (backend) {
    case BackendPreference::Auto: return "auto";
    case BackendPreference::Headless: return "headless";
    case BackendPreference::Drm: return "drm";
    }
    return "auto";
}

void load_config_file(const std::filesystem::path& path, Config& config) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("cannot open configuration file " + path.string());
    }
    std::string section;
    std::string line;
    std::size_t line_number = 0;
    while (std::getline(input, line)) {
        ++line_number;
        bool quoted = false;
        char quote = 0;
        for (std::size_t i = 0; i < line.size(); ++i) {
            const char c = line[i];
            if (quoted) {
                if (quote == '"' && c == '\\') {
                    ++i;
                } else if (c == quote) {
                    quoted = false;
                }
            } else if (c == '"' || c == '\'') {
                quoted = true;
                quote = c;
            } else if (c == '#') {
                line.resize(i);
                break;
            }
        }
        line = trim(line);
        if (line.empty()) {
            continue;
        }
        if (line.front() == '[') {
            if (line.size() < 3 || line.back() != ']') {
                fail(path, line_number, "invalid section header");
            }
            section = trim(std::string_view(line).substr(1, line.size() - 2));
            if (section != "compositor" && section != "output" &&
                    section != "window" && section != "input") {
                fail(path, line_number, "unknown section [" + section + "]");
            }
            continue;
        }
        if (section.empty()) {
            fail(path, line_number, "keys must be inside a section");
        }
        const auto [key, value] = key_value(line, path, line_number);
        apply(section, key, value, config, path, line_number);
    }
}

void load_default_config(Config& config) {
    load_if_present("/etc/materialized/matcomp.toml", config);
    const char* xdg = std::getenv("XDG_CONFIG_HOME");
    const char* home = std::getenv("HOME");
    if (xdg && *xdg) {
        load_if_present(std::filesystem::path(xdg) / "materialized/matcomp.toml", config);
    } else if (home && *home) {
        load_if_present(std::filesystem::path(home) / ".config/materialized/matcomp.toml", config);
    }
}

void print_config(const Config& config, std::ostream& stream) {
    stream << "[compositor]\nbackend = \"" << to_string(config.backend) << "\"\n\n"
           << "[output]\nwidth = " << config.headless_width
           << "\nheight = " << config.headless_height
           << "\nmode = \"" << config.output_mode << "\"\n\n"
           << "[window]\ngap = " << config.window_gap << "\n\n"
           << "[input]\nxkb_layout = \"" << config.xkb_layout
           << "\"\nxkb_model = \"" << config.xkb_model
           << "\"\nxkb_variant = \"" << config.xkb_variant
           << "\"\nxkb_options = \"" << config.xkb_options
           << "\"\nrepeat_rate = " << config.repeat_rate
           << "\nrepeat_delay = " << config.repeat_delay
           << "\nfocus_follows_mouse = "
           << (config.focus_follows_mouse ? "true" : "false") << '\n';
}

} // namespace matcomp
