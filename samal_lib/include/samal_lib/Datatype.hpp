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
  string,
  struct_,
  enum_,
  tuple,
};

class Datatype {
 public:
 private:
  DatatypeCategory mCategory;
  std::variant<
    std::monostate,
    sp<class EnumDeclarationNode>,
    sp<class StructDeclarationNode>,
    std::vector<Datatype>> mFurtherInfo;
};

}