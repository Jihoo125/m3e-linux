#include "matinstall/json.hpp"

#include <cerrno>
#include <charconv>
#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace matinstall::json {
namespace {

void append_utf8(std::string& output, std::uint32_t codepoint) {
  if (codepoint <= 0x7fU) {
    output.push_back(static_cast<char>(codepoint));
  } else if (codepoint <= 0x7ffU) {
    output.push_back(static_cast<char>(0xc0U | (codepoint >> 6U)));
    output.push_back(static_cast<char>(0x80U | (codepoint & 0x3fU)));
  } else if (codepoint <= 0xffffU) {
    output.push_back(static_cast<char>(0xe0U | (codepoint >> 12U)));
    output.push_back(static_cast<char>(0x80U | ((codepoint >> 6U) & 0x3fU)));
    output.push_back(static_cast<char>(0x80U | (codepoint & 0x3fU)));
  } else {
    output.push_back(static_cast<char>(0xf0U | (codepoint >> 18U)));
    output.push_back(static_cast<char>(0x80U | ((codepoint >> 12U) & 0x3fU)));
    output.push_back(static_cast<char>(0x80U | ((codepoint >> 6U) & 0x3fU)));
    output.push_back(static_cast<char>(0x80U | (codepoint & 0x3fU)));
  }
}

class Parser {
 public:
  explicit Parser(std::string_view input) : input_(input) {}

  ParseResult run() {
    ParseResult result;
    try {
      skip_space();
      result.value = parse_value(0);
      skip_space();
      if (position_ != input_.size()) fail("unexpected data after JSON value");
      result.ok = true;
    } catch (const ParseError& error) {
      result.ok = false;
      result.error = error.what();
      result.offset = position_;
    }
    return result;
  }

 private:
  struct ParseError : std::runtime_error {
    using std::runtime_error::runtime_error;
  };

  [[noreturn]] void fail(const std::string& message) const { throw ParseError(message); }

  void skip_space() {
    while (position_ < input_.size()) {
      const char c = input_[position_];
      if (c != ' ' && c != '\t' && c != '\n' && c != '\r') break;
      ++position_;
    }
  }

  bool consume(char expected) {
    if (position_ >= input_.size() || input_[position_] != expected) return false;
    ++position_;
    return true;
  }

  void literal(std::string_view value) {
    if (input_.substr(position_, value.size()) != value) fail("invalid JSON literal");
    position_ += value.size();
  }

  Value parse_value(std::size_t depth) {
    if (depth > 256) fail("JSON nesting is too deep");
    if (position_ >= input_.size()) fail("expected a JSON value");
    const char c = input_[position_];
    if (c == '{') return parse_object(depth + 1);
    if (c == '[') return parse_array(depth + 1);
    if (c == '"') return Value(parse_string());
    if (c == 't') {
      literal("true");
      return Value(true);
    }
    if (c == 'f') {
      literal("false");
      return Value(false);
    }
    if (c == 'n') {
      literal("null");
      return Value(nullptr);
    }
    if (c == '-' || (c >= '0' && c <= '9')) return Value(parse_number());
    fail("expected a JSON value");
  }

  Value parse_object(std::size_t depth) {
    consume('{');
    Value result = Value::object();
    skip_space();
    if (consume('}')) return result;
    while (true) {
      skip_space();
      if (position_ >= input_.size() || input_[position_] != '"') {
        fail("expected a quoted object key");
      }
      std::string key = parse_string();
      skip_space();
      if (!consume(':')) fail("expected ':' after object key");
      skip_space();
      if (result.as_object().contains(key)) fail("duplicate object key: " + key);
      result.set(std::move(key), parse_value(depth));
      skip_space();
      if (consume('}')) return result;
      if (!consume(',')) fail("expected ',' or '}' in object");
    }
  }

  Value parse_array(std::size_t depth) {
    consume('[');
    Value result = Value::array();
    skip_space();
    if (consume(']')) return result;
    while (true) {
      skip_space();
      result.push(parse_value(depth));
      skip_space();
      if (consume(']')) return result;
      if (!consume(',')) fail("expected ',' or ']' in array");
    }
  }

  std::uint32_t hex4() {
    if (input_.size() - position_ < 4) fail("truncated Unicode escape");
    std::uint32_t result = 0;
    for (int i = 0; i < 4; ++i) {
      const char c = input_[position_++];
      result <<= 4U;
      if (c >= '0' && c <= '9') result |= static_cast<std::uint32_t>(c - '0');
      else if (c >= 'a' && c <= 'f') result |= static_cast<std::uint32_t>(c - 'a' + 10);
      else if (c >= 'A' && c <= 'F') result |= static_cast<std::uint32_t>(c - 'A' + 10);
      else fail("invalid Unicode escape");
    }
    return result;
  }

  std::string parse_string() {
    if (!consume('"')) fail("expected a string");
    std::string result;
    while (position_ < input_.size()) {
      const unsigned char c = static_cast<unsigned char>(input_[position_++]);
      if (c == '"') return result;
      if (c < 0x20U) fail("unescaped control character in string");
      if (c == '\\') {
        if (position_ >= input_.size()) fail("truncated string escape");
        const char escaped = input_[position_++];
        switch (escaped) {
          case '"': result.push_back('"'); break;
          case '\\': result.push_back('\\'); break;
          case '/': result.push_back('/'); break;
          case 'b': result.push_back('\b'); break;
          case 'f': result.push_back('\f'); break;
          case 'n': result.push_back('\n'); break;
          case 'r': result.push_back('\r'); break;
          case 't': result.push_back('\t'); break;
          case 'u': {
            std::uint32_t codepoint = hex4();
            if (codepoint >= 0xd800U && codepoint <= 0xdbffU) {
              if (input_.substr(position_, 2) != "\\u") fail("missing low Unicode surrogate");
              position_ += 2;
              const std::uint32_t low = hex4();
              if (low < 0xdc00U || low > 0xdfffU) fail("invalid low Unicode surrogate");
              codepoint = 0x10000U + ((codepoint - 0xd800U) << 10U) + (low - 0xdc00U);
            } else if (codepoint >= 0xdc00U && codepoint <= 0xdfffU) {
              fail("unexpected low Unicode surrogate");
            }
            append_utf8(result, codepoint);
            break;
          }
          default: fail("invalid string escape");
        }
        continue;
      }
      if (c < 0x80U) {
        result.push_back(static_cast<char>(c));
        continue;
      }
      // Validate and retain UTF-8. This prevents arbitrary bytes from reaching
      // paths, logs, locale files, or argv through the answer parser.
      int length = 0;
      std::uint32_t codepoint = 0;
      std::uint32_t minimum = 0;
      if ((c & 0xe0U) == 0xc0U) {
        length = 2; codepoint = c & 0x1fU; minimum = 0x80U;
      } else if ((c & 0xf0U) == 0xe0U) {
        length = 3; codepoint = c & 0x0fU; minimum = 0x800U;
      } else if ((c & 0xf8U) == 0xf0U) {
        length = 4; codepoint = c & 0x07U; minimum = 0x10000U;
      } else {
        fail("invalid UTF-8 leading byte");
      }
      result.push_back(static_cast<char>(c));
      for (int index = 1; index < length; ++index) {
        if (position_ >= input_.size()) fail("truncated UTF-8 sequence");
        const unsigned char continuation = static_cast<unsigned char>(input_[position_]);
        if ((continuation & 0xc0U) != 0x80U) fail("invalid UTF-8 continuation byte");
        result.push_back(input_[position_++]);
        codepoint = (codepoint << 6U) | (continuation & 0x3fU);
      }
      if (codepoint < minimum || codepoint > 0x10ffffU ||
          (codepoint >= 0xd800U && codepoint <= 0xdfffU)) {
        fail("non-canonical or invalid UTF-8 codepoint");
      }
    }
    fail("unterminated string");
  }

  Value parse_number() {
    const std::size_t start = position_;
    consume('-');
    if (consume('0')) {
      if (position_ < input_.size() && input_[position_] >= '0' && input_[position_] <= '9') {
        fail("leading zero in number");
      }
    } else {
      if (position_ >= input_.size() || input_[position_] < '1' || input_[position_] > '9') {
        fail("invalid number");
      }
      while (position_ < input_.size() && input_[position_] >= '0' && input_[position_] <= '9') {
        ++position_;
      }
    }
    if (consume('.')) {
      const std::size_t digits = position_;
      while (position_ < input_.size() && input_[position_] >= '0' && input_[position_] <= '9') {
        ++position_;
      }
      if (digits == position_) fail("expected digits after decimal point");
    }
    if (position_ < input_.size() && (input_[position_] == 'e' || input_[position_] == 'E')) {
      ++position_;
      if (position_ < input_.size() && (input_[position_] == '+' || input_[position_] == '-')) {
        ++position_;
      }
      const std::size_t digits = position_;
      while (position_ < input_.size() && input_[position_] >= '0' && input_[position_] <= '9') {
        ++position_;
      }
      if (digits == position_) fail("expected exponent digits");
    }
    return Value(Number{std::string(input_.substr(start, position_ - start))});
  }

  std::string_view input_;
  std::size_t position_ = 0;
};

void print_value(const Value& value, std::string& output, int indent, int depth) {
  if (value.is_null()) {
    output += "null";
  } else if (value.is_bool()) {
    output += value.as_bool() ? "true" : "false";
  } else if (value.is_number()) {
    output += value.as_number().text;
  } else if (value.is_string()) {
    output += escape_string(value.as_string());
  } else if (value.is_array()) {
    const auto& values = value.as_array();
    output.push_back('[');
    for (std::size_t index = 0; index < values.size(); ++index) {
      if (index != 0) output.push_back(',');
      if (indent > 0) {
        output.push_back('\n');
        output.append(static_cast<std::size_t>((depth + 1) * indent), ' ');
      }
      print_value(values[index], output, indent, depth + 1);
    }
    if (!values.empty() && indent > 0) {
      output.push_back('\n');
      output.append(static_cast<std::size_t>(depth * indent), ' ');
    }
    output.push_back(']');
  } else {
    const auto& values = value.as_object();
    output.push_back('{');
    std::size_t index = 0;
    for (const auto& [key, child] : values) {
      if (index++ != 0) output.push_back(',');
      if (indent > 0) {
        output.push_back('\n');
        output.append(static_cast<std::size_t>((depth + 1) * indent), ' ');
      }
      output += escape_string(key);
      output += indent > 0 ? ": " : ":";
      print_value(child, output, indent, depth + 1);
    }
    if (!values.empty() && indent > 0) {
      output.push_back('\n');
      output.append(static_cast<std::size_t>(depth * indent), ' ');
    }
    output.push_back('}');
  }
}

}  // namespace

Value Value::number(std::uint64_t value) {
  Value result;
  result.value_ = Number{std::to_string(value)};
  return result;
}

Value Value::number(std::int64_t value) {
  Value result;
  result.value_ = Number{std::to_string(value)};
  return result;
}

Value Value::array() {
  Value result;
  result.value_ = Array{};
  return result;
}

Value Value::object() {
  Value result;
  result.value_ = Object{};
  return result;
}

bool Value::is_null() const { return std::holds_alternative<std::nullptr_t>(value_); }
bool Value::is_bool() const { return std::holds_alternative<bool>(value_); }
bool Value::is_number() const { return std::holds_alternative<Number>(value_); }
bool Value::is_string() const { return std::holds_alternative<std::string>(value_); }
bool Value::is_array() const { return std::holds_alternative<Array>(value_); }
bool Value::is_object() const { return std::holds_alternative<Object>(value_); }

bool Value::as_bool() const {
  if (!is_bool()) throw std::runtime_error("JSON value is not a boolean");
  return std::get<bool>(value_);
}

const std::string& Value::as_string() const {
  if (!is_string()) throw std::runtime_error("JSON value is not a string");
  return std::get<std::string>(value_);
}

const Number& Value::as_number() const {
  if (!is_number()) throw std::runtime_error("JSON value is not a number");
  return std::get<Number>(value_);
}

const Value::Array& Value::as_array() const {
  if (!is_array()) throw std::runtime_error("JSON value is not an array");
  return std::get<Array>(value_);
}

const Value::Object& Value::as_object() const {
  if (!is_object()) throw std::runtime_error("JSON value is not an object");
  return std::get<Object>(value_);
}

Value::Array& Value::as_array() {
  if (!is_array()) throw std::runtime_error("JSON value is not an array");
  return std::get<Array>(value_);
}

Value::Object& Value::as_object() {
  if (!is_object()) throw std::runtime_error("JSON value is not an object");
  return std::get<Object>(value_);
}

const Value* Value::find(std::string_view key) const {
  if (!is_object()) return nullptr;
  const auto& object = std::get<Object>(value_);
  const auto found = object.find(key);
  return found == object.end() ? nullptr : &found->second;
}

Value& Value::set(std::string key, Value value) {
  auto& object = as_object();
  auto [iterator, inserted] = object.insert_or_assign(std::move(key), std::move(value));
  (void)inserted;
  return iterator->second;
}

void Value::push(Value value) { as_array().push_back(std::move(value)); }

ParseResult parse(std::string_view text) { return Parser(text).run(); }

Value parse_or_throw(std::string_view text) {
  ParseResult result = parse(text);
  if (!result.ok) {
    throw std::runtime_error("JSON at byte " + std::to_string(result.offset) + ": " + result.error);
  }
  return std::move(result.value);
}

std::string stringify(const Value& value, int indent) {
  if (indent < 0) indent = 0;
  std::string result;
  print_value(value, result, indent, 0);
  result.push_back('\n');
  return result;
}

bool number_to_i64(const Value& value, std::int64_t& result) {
  if (!value.is_number()) return false;
  const auto& text = value.as_number().text;
  const char* first = text.data();
  const char* last = first + text.size();
  const auto conversion = std::from_chars(first, last, result);
  return conversion.ec == std::errc{} && conversion.ptr == last;
}

bool number_to_u64(const Value& value, std::uint64_t& result) {
  if (!value.is_number() || (!value.as_number().text.empty() && value.as_number().text[0] == '-')) {
    return false;
  }
  const auto& text = value.as_number().text;
  const char* first = text.data();
  const char* last = first + text.size();
  const auto conversion = std::from_chars(first, last, result);
  return conversion.ec == std::errc{} && conversion.ptr == last;
}

std::string escape_string(std::string_view text) {
  std::string result = "\"";
  constexpr char digits[] = "0123456789abcdef";
  for (const char byte : text) {
    const unsigned char c = static_cast<unsigned char>(byte);
    switch (c) {
      case '"': result += "\\\""; break;
      case '\\': result += "\\\\"; break;
      case '\b': result += "\\b"; break;
      case '\f': result += "\\f"; break;
      case '\n': result += "\\n"; break;
      case '\r': result += "\\r"; break;
      case '\t': result += "\\t"; break;
      default:
        if (c < 0x20U) {
          result += "\\u00";
          result.push_back(digits[(c >> 4U) & 0x0fU]);
          result.push_back(digits[c & 0x0fU]);
        } else {
          result.push_back(static_cast<char>(c));
        }
    }
  }
  result.push_back('"');
  return result;
}

}  // namespace matinstall::json
