#pragma once
#include <samal_lib/Util.hpp>
#include <variant>
#include <vector>
#include <compare>

namespace samal {

enum class DatatypeCategory {
  i64,
  i32,
  f64,
  f32,
  char_,
  undetermined_identifier,
  string,
  struct_,
  enum_,
  tuple,
  function,
};

class Datatype {
 public:
  explicit Datatype(DatatypeCategory category);
  Datatype(Datatype returnType, std::vector<Datatype> params);
  explicit Datatype(std::string identifierName);
  [[nodiscard]] std::string toString() const;
  [[nodiscard]] const std::pair<sp<Datatype>, std::vector<Datatype>>& getFunctionTypeInfo() const;
  bool operator==(const Datatype& other) const;
  bool operator!=(const Datatype& other) const;
  [[nodiscard]] DatatypeCategory getCategory() const;
 private:
  DatatypeCategory mCategory;
  std::variant<
    std::monostate,
    std::string,
    sp<class EnumDeclarationNode>,
    sp<class StructDeclarationNode>,
    std::pair<sp<Datatype>, std::vector<Datatype>>> mFurtherInfo;
};

}