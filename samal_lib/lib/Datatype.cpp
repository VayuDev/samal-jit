#include "samal_lib/Datatype.hpp"
#include "samal_lib/AST.hpp"
#include <cassert>

namespace samal {

std::string Datatype::toString() const {
    auto funcToString = [this] {
        auto& functionInfo = getFunctionTypeInfo();
        std::string ret = "fn(";
        for(size_t i = 0; i < functionInfo.second.size(); ++i) {
            ret += functionInfo.second.at(i).toString();
            if(i < functionInfo.second.size() - 1) {
                ret += ",";
            }
        }
        ret += ") -> " + functionInfo.first.toString();
        return ret;
    };
    switch(mCategory) {
    case DatatypeCategory::i32:
        return "i32";
    case DatatypeCategory::i64:
        return "i64";
    case DatatypeCategory::function: {
        return funcToString();
    }
    case DatatypeCategory::tuple: {
        auto& tupleInfo = getTupleInfo();
        std::string ret = "(";
        for(size_t i = 0; i < tupleInfo.size(); ++i) {
            ret += tupleInfo.at(i).toString();
            if(i < tupleInfo.size() - 1) {
                ret += ",";
            }
        }
        ret += ")";
        return ret;
    }
    case DatatypeCategory::list: {
        return "[" + getListInfo().toString() + "]";
    }
    case DatatypeCategory::bool_:
        return "bool";
    case DatatypeCategory::undetermined_identifier:
        return "<undetermined '" + getUndeterminedIdentifierString() + "'>";
    case DatatypeCategory::struct_:
        return "<struct " + getStructInfo().name + ">";
    default:
        return "DATATYPE";
    }
}
Datatype::Datatype()
: mCategory(DatatypeCategory::invalid) {
}
Datatype Datatype::createEmptyTuple() {
    return Datatype(DatatypeCategory::tuple, std::vector<Datatype>{});
}
Datatype Datatype::createListType(Datatype baseType) {
    return Datatype(DatatypeCategory::list, std::move(baseType));
}
Datatype Datatype::createStructType(const std::string& name, const std::vector<Parameter>& params, std::vector<std::string> templateParams) {
    std::vector<StructInfo::StructElement> elements;
    for(const auto& p : params) {
        elements.push_back(StructInfo::StructElement{ .name = p.name->getName(), .baseType = p.type });
    }
    std::sort(elements.begin(), elements.end(), [](auto& a, auto& b) -> bool {
        return a.name < b.name;
    });
    return Datatype(DatatypeCategory::struct_, StructInfo{ .name = name, .elements = std::move(elements), .templateParams = std::move(templateParams) });
}
Datatype::Datatype(DatatypeCategory cat, ContainedFurtherInfoType furtherInfo)
: mFurtherInfo(std::make_unique<ContainedFurtherInfoType>(std::move(furtherInfo))), mCategory(cat) {
}
Datatype Datatype::createSimple(DatatypeCategory category) {
    return Datatype(category, std::monostate{});
}
Datatype Datatype::createFunctionType(Datatype returnType, std::vector<Datatype> params) {
    return Datatype(DatatypeCategory::function, std::make_pair<Datatype, std::vector<Datatype>>(std::move(returnType), std::move(params)));
}
Datatype Datatype::createTupleType(std::vector<Datatype> params) {
    return Datatype(DatatypeCategory::tuple, std::move(params));
}
Datatype Datatype::createUndeterminedIdentifierType(std::string name) {
    return Datatype(DatatypeCategory::undetermined_identifier, IdentifierInfo{std::move(name), {}});
}
Datatype Datatype::createUndeterminedIdentifierType(const IdentifierNode& name) {
  return Datatype(DatatypeCategory::undetermined_identifier, IdentifierInfo{name.getName(), name.getTemplateParameters()});
}
const std::pair<Datatype, std::vector<Datatype>>& Datatype::getFunctionTypeInfo() const& {
    if(mCategory != DatatypeCategory::function) {
        throw std::runtime_error{ "Can't call this type!" };
    }
    return std::get<std::pair<Datatype, std::vector<Datatype>>>(*mFurtherInfo);
}
const std::vector<Datatype>& Datatype::getTupleInfo() const {
    if(mCategory != DatatypeCategory::tuple) {
        throw std::runtime_error{ "This is not a tuple!" };
    }
    return std::get<std::vector<Datatype>>(*mFurtherInfo);
}
const Datatype& Datatype::getListInfo() const {
    if(mCategory != DatatypeCategory::list) {
        throw std::runtime_error{ "This is not a list!" };
    }
    return std::get<Datatype>(*mFurtherInfo);
}
const std::string& Datatype::getUndeterminedIdentifierString() const {
    if(mCategory != DatatypeCategory::undetermined_identifier) {
        throw std::runtime_error{ "This is not an undetermined identifier!" };
    }
    return std::get<IdentifierInfo>(*mFurtherInfo).name;
}
const std::vector<Datatype>& Datatype::getUndeterminedIdentifierTemplateParams() const {
    if(mCategory != DatatypeCategory::undetermined_identifier) {
        throw std::runtime_error{ "This is not an undetermined identifier!" };
    }
    return std::get<IdentifierInfo>(*mFurtherInfo).templateParams;
}
const Datatype::StructInfo& Datatype::getStructInfo() const {
    if(mCategory != DatatypeCategory::struct_) {
        throw std::runtime_error{ "This is not a struct!" };
    }
    return std::get<StructInfo>(*mFurtherInfo);
}
bool Datatype::operator==(const Datatype& other) const {
    if(mCategory != other.mCategory)
        return false;
    if(mFurtherInfo->index() != other.mFurtherInfo->index())
        return false;
    if(*mFurtherInfo != *other.mFurtherInfo) {
        return false;
    }
    return true;
}
bool Datatype::operator!=(const Datatype& other) const {
    return !(*this == other);
}
DatatypeCategory Datatype::getCategory() const {
    return mCategory;
}
bool Datatype::isInteger() const {
    return mCategory == DatatypeCategory::i32 || mCategory == DatatypeCategory::i64;
}
size_t Datatype::getSizeOnStack() const {
#ifdef x86_64_BIT_MODE
    switch(mCategory) {
    case DatatypeCategory::i32:
    case DatatypeCategory::i64:
    case DatatypeCategory::function:
    case DatatypeCategory::bool_:
    case DatatypeCategory::list:
    case DatatypeCategory::struct_:
        return 8;
    case DatatypeCategory::tuple: {
        size_t sum = 0;
        for(auto& subType : getTupleInfo()) {
            sum += subType.getSizeOnStack();
        }
        return sum;
    }
    default:
        assert(false);
    }
#endif
    switch(mCategory) {
    case DatatypeCategory::i32:
        return 4;
    case DatatypeCategory::i64:
    case DatatypeCategory::function:
    case DatatypeCategory::list:
    case DatatypeCategory::struct_:
        return 8;
    case DatatypeCategory::bool_:
        return 1;
    case DatatypeCategory::tuple: {
        size_t sum = 0;
        for(auto& subType : getTupleInfo()) {
            sum += subType.getSizeOnStack();
        }
        return sum;
    }
    default:
        assert(false);
    }
}
Datatype Datatype::completeWithTemplateParameters(const std::map<std::string, Datatype>& templateParams) const {
    return completeWithTemplateParameters(templateParams, InternalCall::No);
}
void Datatype::attachUndeterminedIdentifierMap(sp<std::map<std::string, Datatype>> map) {
    mUndefinedTypeReplacementMap = map;
    switch(mCategory) {
    case DatatypeCategory::bool_:
    case DatatypeCategory::i32:
    case DatatypeCategory::i64:
    case DatatypeCategory::string:
    case DatatypeCategory::undetermined_identifier:
        break;
    case DatatypeCategory::function: {
        auto& funcType = std::get<std::pair<Datatype, std::vector<Datatype>>>(*mFurtherInfo);
        funcType.first.attachUndeterminedIdentifierMap(map);
        for(auto& param : funcType.second) {
            param.attachUndeterminedIdentifierMap(map);
        }
        break;
    }
    case DatatypeCategory::tuple: {
        for(size_t i = 0; i < getTupleInfo().size(); ++i) {
            std::get<std::vector<Datatype>>(*mFurtherInfo).at(i).attachUndeterminedIdentifierMap(map);
        }
        break;
    }
    case DatatypeCategory::list: {
        std::get<Datatype>(*mFurtherInfo).attachUndeterminedIdentifierMap(map);
        break;
    }
    case DatatypeCategory::struct_: {
        for(size_t i = 0; i < getStructInfo().elements.size(); ++i) {
            std::get<StructInfo>(*mFurtherInfo).elements.at(i).baseType.attachUndeterminedIdentifierMap(map);
        }
        break;
    }
    default:
        assert(false);
    }
}
Datatype::Datatype(const Datatype& other) {
    operator=(other);
}
Datatype& Datatype::operator=(const Datatype& other) {
    if(this == &other)
        return *this;
    mCategory = other.mCategory;
    mUndefinedTypeReplacementMap = other.mUndefinedTypeReplacementMap;
    mFurtherInfo = std::make_unique<ContainedFurtherInfoType>(*other.mFurtherInfo);
    return *this;
}
Datatype Datatype::completeWithSavedTemplateParameters() const {
    assert(mUndefinedTypeReplacementMap);
    return completeWithTemplateParameters(*mUndefinedTypeReplacementMap, InternalCall::Yes);
}
Datatype Datatype::completeWithTemplateParameters(const std::map<std::string, Datatype>& templateParams, Datatype::InternalCall internalCall) const {
    // This assertion ensures that we don't accidentally call completeWithTemplateParameters twice, overwriting the mUndefinedTypeReplacementMap
    assert(!(internalCall == InternalCall::No && mUndefinedTypeReplacementMap));
    Datatype cpy = *this;
    switch(mCategory) {
    case DatatypeCategory::bool_:
    case DatatypeCategory::i32:
    case DatatypeCategory::i64:
    case DatatypeCategory::string:
        break;
    case DatatypeCategory::function: {
        const auto& ourFunctionType = getFunctionTypeInfo();
        auto& cpyFunctionType = std::get<std::pair<Datatype, std::vector<Datatype>>>(*cpy.mFurtherInfo);
        cpyFunctionType.first = ourFunctionType.first.completeWithTemplateParameters(templateParams, internalCall);
        size_t i = 0;
        for(auto& param : ourFunctionType.second) {
            cpyFunctionType.second.at(i) = param.completeWithTemplateParameters(templateParams, internalCall);
            ++i;
        }
        break;
    }
    case DatatypeCategory::undetermined_identifier: {
        auto maybeReplacementType = templateParams.find(getUndeterminedIdentifierString());
        if(maybeReplacementType != templateParams.end()) {
            cpy = maybeReplacementType->second;
            if(cpy.getCategory() == DatatypeCategory::struct_) {
                // add template params to replacement map
                std::vector<Datatype> undeterminedIdentifierTemplateParameters;
                for(auto& dt: getUndeterminedIdentifierTemplateParams()) {
                    undeterminedIdentifierTemplateParameters.emplace_back(dt.completeWithTemplateParameters(templateParams, internalCall));
                }
                auto additionalMap = createTemplateParamMap(cpy.getStructInfo().templateParams, undeterminedIdentifierTemplateParameters);
                additionalMap.insert(templateParams.cbegin(), templateParams.cend());
                cpy.attachUndeterminedIdentifierMap(std::make_shared<std::map<std::string, Datatype>>(additionalMap));
                return cpy;
            }
        }
        break;
    }
    case DatatypeCategory::tuple: {
        for(size_t i = 0; i < getTupleInfo().size(); ++i) {
            std::get<std::vector<Datatype>>(*cpy.mFurtherInfo).at(i) = getTupleInfo().at(i).completeWithTemplateParameters(templateParams, internalCall);
        }
        break;
    }
    case DatatypeCategory::list: {
        std::get<Datatype>(*cpy.mFurtherInfo) = getListInfo().completeWithTemplateParameters(templateParams, internalCall);
        break;
    }
    case DatatypeCategory::struct_: {
        size_t i = 0;
        for(auto& element : getStructInfo().elements) {
            std::get<StructInfo>(*cpy.mFurtherInfo).elements.at(i).baseType = getStructInfo().elements.at(i).baseType.completeWithTemplateParameters(templateParams, internalCall);
            ++i;
        }
        break;
    }
    default:
        assert(false);
    }
    cpy.attachUndeterminedIdentifierMap(std::make_shared<std::map<std::string, Datatype>>(templateParams));
    return cpy;
}

}