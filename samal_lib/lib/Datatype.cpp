#include "samal_lib/Datatype.hpp"

namespace samal {

std::string Datatype::toString() const {
  switch(mCategory) {
    case DatatypeCategory::i32:
      return "i32";
  }
  return "DATATYPE";
}
Datatype::Datatype(DatatypeCategory category)
: mCategory(category) {

}

}