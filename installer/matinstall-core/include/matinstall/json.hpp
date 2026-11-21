#pragma once

#include <cstdint>
#include <initializer_list>
#include <map>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace matinstall::json {

struct Number {
  std::string text;
};

class Value {
 public:
  using Array = std::vector<Value>;
  using Object = std::map<std::string, Value, std::less<>>;
  using Storage = std::variant<std::nullptr_t, bool, Number, std::string, Array, Object>;

  Value() : value_(nullptr) {}
  Value(std::nullptr_t) : value_(nullptr) {}
  Value(bool value) : value_(value) {}
  Value(const char* value) : value_(std::string(value)) {}
  Value(std::string value) : value_(std::move(value)) {}
  Value(Number value) : value_(std::move(value)) {}

  static Value number(std::uint64_t value);
  static Value number(std::int64_t value);
  static Value array();
  static Value object();

  bool is_null() const;
  bool is_bool() const;
  bool is_number() const;
  bool is_string() const;
  bool is_array() const;
  bool is_object() const;

  bool as_bool() const;
  const std::string& as_string() const;
  const Number& as_number() const;
  const Array& as_array() const;
  const Object& as_object() const;
  Array& as_array();
  Object& as_object();

  const Value* find(std::string_view key) const;
  Value& set(std::string key, Value value);
  void push(Value value);

  const Storage& storage() const { return value_; }

 private:
  Storage value_;
};

struct ParseResult {
  bool ok = false;
  Value value;
  std::string error;
  std::size_t offset = 0;
};

ParseResult parse(std::string_view text);
Value parse_or_throw(std::string_view text);
std::string stringify(const Value& value, int indent = 2);

bool number_to_u64(const Value& value, std::uint64_t& result);
bool number_to_i64(const Value& value, std::int64_t& result);
std::string escape_string(std::string_view text);

}  // namespace matinstall::json
