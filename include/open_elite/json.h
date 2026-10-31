#ifndef OPEN_ELITE_JSON_H_
#define OPEN_ELITE_JSON_H_

#include <cstddef>
#include <string>
#include <vector>

namespace open_elite {

/// A JSON value. Objects keep their members in the order they were set, so output is deterministic.
class Json {
 public:
  enum class Type { kNull, kBool, kNumber, kString, kArray, kObject };

  Json() = default;

  static Json Bool(bool value);
  static Json Number(double value);
  static Json String(std::string value);
  static Json MakeArray();
  static Json MakeObject();

  Type type() const { return type_; }
  bool is_null() const { return type_ == Type::kNull; }
  bool is_bool() const { return type_ == Type::kBool; }
  bool is_number() const { return type_ == Type::kNumber; }
  bool is_string() const { return type_ == Type::kString; }
  bool is_array() const { return type_ == Type::kArray; }
  bool is_object() const { return type_ == Type::kObject; }

  bool bool_value() const { return bool_; }
  double number_value() const { return number_; }
  const std::string& string_value() const { return string_; }

  /// The elements of an array, or the member values of an object
  const std::vector<Json>& items() const { return items_; }
  /// The member names of an object, in the same order as items()
  const std::vector<std::string>& keys() const { return keys_; }

  /// Append to an array
  void Push(Json value);
  /// Add a member to an object, replacing one of the same name
  void Set(const std::string& key, Json value);
  /// A member of an object, or nullptr
  const Json* Find(const std::string& key) const;

  /// Text form. Containers holding only plain values are written on one line.
  std::string Dump() const;

  /// Parse text. Returns false and describes the problem in *error (if not null) when the text is not valid JSON.
  static bool Parse(const std::string& text, Json* out, std::string* error);

 private:
  Type type_ = Type::kNull;
  bool bool_ = false;
  double number_ = 0.0;
  std::string string_;
  std::vector<std::string> keys_;
  std::vector<Json> items_;
};

}  // namespace open_elite

#endif  // OPEN_ELITE_JSON_H_
