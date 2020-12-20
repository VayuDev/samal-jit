#pragma once
#include <samal_lib/Util.hpp>
#include <variant>
#include <vector>

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
  [[nodiscard]] std::string toString() const;
  [[nodiscard]] const std::pair<sp<Datatype>, std::vector<Datatype>>& getFunctionTypeInfo() const;
 private:
  DatatypeCategory mCategory;
  std::variant<
    std::monostate,
    sp<class EnumDeclarationNode>,
    sp<class StructDeclarationNode>,
    std::pair<sp<Datatype>, std::vector<Datatype>>> mFurtherInfo;
};

}