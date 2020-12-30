#include "samal_lib/Datatype.hpp"

namespace samal {

std::string Datatype::toString() const {
  switch(mCategory) {
  case DatatypeCategory::i32:
    return "i32";
  case DatatypeCategory::function: {
    auto &functionInfo = getFunctionTypeInfo();
    std::string ret = "fn(";
    for (size_t i = 0; i < functionInfo.second.size(); ++i) {
      ret += functionInfo.second.at(i).toString();
      if (i < functionInfo.second.size() - 1) {
        ret += ",";
      }
    }
    ret += ") -> " + functionInfo.first->toString();
    return ret;
  }
  case DatatypeCategory::tuple: {
    auto &tupleInfo = getTupleInfo();
    std::string ret = "(";
    for (size_t i = 0; i < tupleInfo.size(); ++i) {
      ret += tupleInfo.at(i).toString();
      if (i < tupleInfo.size() - 1) {
        ret += ",";
      }
    }
    ret += ")";
    return ret;
  }
  case DatatypeCategory::undetermined_identifier:
    return "<undetermined identifier '" + std::get<std::string>(mFurtherInfo) + "'>";
  default:
    return "DATATYPE";
  }
}
Datatype::Datatype(DatatypeCategory category)
: mCategory(category) {

}
Datatype::Datatype(Datatype returnType, std::vector<Datatype> params) {
  mFurtherInfo = std::make_pair(
      std::make_shared<Datatype>(std::move(returnType)),
      std::move(params));
  mCategory = DatatypeCategory::function;
}
Datatype::Datatype(std::string identifierName)
: mFurtherInfo(std::move(identifierName)), mCategory(DatatypeCategory::undetermined_identifier) {

}
Datatype::Datatype(std::vector<Datatype> params)
    : mFurtherInfo(std::move(params)), mCategory(DatatypeCategory::tuple) {

}
const std::pair<sp<Datatype>, std::vector<Datatype>>&  Datatype::getFunctionTypeInfo() const {
  if(mCategory != DatatypeCategory::function) {
    throw std::runtime_error{"Can't call this type!"};
  }
  return std::get<std::pair<sp<Datatype>, std::vector<Datatype>>>(mFurtherInfo);
}
const std::vector<Datatype> &Datatype::getTupleInfo() const {
  if(mCategory != DatatypeCategory::tuple) {
    throw std::runtime_error{"This is not a tuple!"};
  }
  return std::get<std::vector<Datatype>>(mFurtherInfo);
}
bool Datatype::operator==(const Datatype &other) const {
  if(mCategory != other.mCategory)
    return false;
  if(mFurtherInfo.index() != other.mFurtherInfo.index())
    return false;
  try {
    auto& selfValue = std::get<std::pair<sp<Datatype>, std::vector<Datatype>>>(mFurtherInfo);
    auto& otherValue =  std::get<std::pair<sp<Datatype>, std::vector<Datatype>>>(other.mFurtherInfo);
    if(selfValue.second != otherValue.second)
      return false;
    if(selfValue.first != otherValue.first) {
      if(selfValue.first && otherValue.first) {
        if(*selfValue.first != *otherValue.first) {
          return false;
        } else {
          return true;
        }
      }
    }
  } catch(std::bad_variant_access&) {
    if(mFurtherInfo != other.mFurtherInfo) {
      return false;
    }
  }
  return true;
}
bool Datatype::operator!=(const Datatype &other) const {
  return !(*this == other);
}
DatatypeCategory Datatype::getCategory() const {
  return mCategory;
}
Datatype Datatype::createEmptyTuple() {
  return Datatype(std::vector<Datatype>{});
}

}