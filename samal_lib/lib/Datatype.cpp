#include "samal_lib/Datatype.hpp"
#include "samal_lib/AST.hpp"
#include <cassert>

namespace samal {

std::string Datatype::toString() const {
    switch(mCategory) {
    case DatatypeCategory::i32:
        return "i32";
    case DatatypeCategory::i64:
        return "i64";
    case DatatypeCategory::function: {auto& functionInfo = getFunctionTypeInfo();
        std::string ret = "fn(";
        for(size_t i = 0; i < functionInfo.second.size(); ++i) {
            ret += functionInfo.second.at(i).toString();
            if(i < functionInfo.second.size() - 1) {
                ret += ",";
            }
        }
        ret += ") -> " + functionInfo.first.toString();
        return ret;
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
        return "[" + getListContainedType().toString() + "]";
    }
    case DatatypeCategory::bool_:
        return "bool";
    case DatatypeCategory::undetermined_identifier:
        return "<undetermined '" + getUndeterminedIdentifierString() + "'>";
    case DatatypeCategory::struct_:
        return "<struct " + getStructInfo().name + ">";
    case DatatypeCategory::enum_:
        return "<enum " + getEnumInfo().name + ">";
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
        elements.push_back(StructInfo::StructElement{ .name = p.name->getName(), .type = p.type });
    }
    return Datatype(DatatypeCategory::struct_, StructInfo{ .name = name, .fields = std::move(elements), .templateParams = std::move(templateParams) });
}
Datatype Datatype::createEnumType(const std::string& name, const std::vector<EnumField>& params, std::vector<std::string> templateParams) {
    return Datatype(DatatypeCategory::enum_, EnumInfo{ .name = name, .fields = params, .templateParams = std::move(templateParams) });
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
    return Datatype(DatatypeCategory::undetermined_identifier, IdentifierInfo{ std::move(name), {} });
}
Datatype Datatype::createUndeterminedIdentifierType(const IdentifierNode& name) {
    return Datatype(DatatypeCategory::undetermined_identifier, IdentifierInfo{ name.getName(), name.getTemplateParameters() });
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
const Datatype::EnumInfo& Datatype::getEnumInfo() const {
    if(mCategory != DatatypeCategory::enum_) {
        throw std::runtime_error{ "This is not an enum!" };
    }
    return std::get<EnumInfo>(*mFurtherInfo);
}
bool Datatype::operator==(const Datatype& other) const {
    if(mCategory == DatatypeCategory::undetermined_identifier && mUndefinedTypeReplacementMap && other.mCategory != DatatypeCategory::undetermined_identifier) {
        return completeWithSavedTemplateParameters() == other;
    }
    if(other.mCategory == DatatypeCategory::undetermined_identifier && mUndefinedTypeReplacementMap && mCategory != DatatypeCategory::undetermined_identifier) {
        return *this == other.completeWithSavedTemplateParameters();
    }
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
bool Datatype::isInteger() const {
    return mCategory == DatatypeCategory::i32 || mCategory == DatatypeCategory::i64;
}
size_t Datatype::getSizeOnStack(int32_t depth) const {
    if(depth > 1000) {
        throw std::runtime_error{"Maximum type recursion level of 1000 reached!"};
    }
#ifdef x86_64_BIT_MODE
    switch(mCategory) {
    case DatatypeCategory::i32:
    case DatatypeCategory::i64:
    case DatatypeCategory::function:
    case DatatypeCategory::bool_:
    case DatatypeCategory::list:
        return 8;
    case DatatypeCategory::enum_:
        return getEnumInfo().getLargestFieldSizePlusIndex(depth + 1);
    case DatatypeCategory::struct_: {
        size_t sum = 0;
        for(auto& field : getStructInfo().fields) {
            sum += field.type.completeWithSavedTemplateParameters().getSizeOnStack(depth + 1);
        }
        return sum;
    }
    case DatatypeCategory::tuple: {
        size_t sum = 0;
        for(auto& subType : getTupleInfo()) {
            sum += subType.completeWithSavedTemplateParameters().getSizeOnStack(depth + 1);
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
        return 8;
    case DatatypeCategory::struct_: {
        size_t sum = 0;
        for(auto& field : getStructInfo().fields) {
            sum += field.type.completeWithSavedTemplateParameters().getSizeOnStack(depth + 1);
        }
        return sum;
    }
    case DatatypeCategory::enum_:
        return getEnumInfo().getLargestFieldSizePlusIndex(depth + 1);
    case DatatypeCategory::bool_:
        return 1;
    case DatatypeCategory::tuple: {
        size_t sum = 0;
        for(auto& subType : getTupleInfo()) {
            sum += subType.getSizeOnStack(depth + 1);
        }
        return sum;
    }
    default:
        assert(false);
    }
}
Datatype Datatype::completeWithTemplateParameters(const std::map<std::string, Datatype>& templateParams, const std::vector<std::string>& modules, AllowIncompleteTypes allowIncompleteTypes) const {
    return completeWithTemplateParameters(templateParams, modules, InternalCall::No, allowIncompleteTypes);
}
void Datatype::attachUndeterminedIdentifierMap(sp<UndeterminedIdentifierCompletionInfo> map) {
    if(mUndefinedTypeReplacementMap)
        return;
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
        for(size_t i = 0; i < getStructInfo().fields.size(); ++i) {
            std::get<StructInfo>(*mFurtherInfo).fields.at(i).type.attachUndeterminedIdentifierMap(map);
        }
        break;
    }
    case DatatypeCategory::enum_: {
        const auto& fields = getEnumInfo().fields;
        for(size_t i = 0; i < fields.size(); ++i) {
            const auto& fieldElements = fields.at(i).params;
            for(size_t j = 0; j < fieldElements.size(); ++j) {
                std::get<EnumInfo>(*mFurtherInfo).fields.at(i).params.at(j).attachUndeterminedIdentifierMap(map);
            }
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
Datatype Datatype::completeWithSavedTemplateParameters(AllowIncompleteTypes allowIncompleteTypes) const {
    if(mUndefinedTypeReplacementMap)
        return completeWithTemplateParameters(mUndefinedTypeReplacementMap->map, mUndefinedTypeReplacementMap->includedModules, InternalCall::Yes, allowIncompleteTypes);
    return *this;
}
Datatype Datatype::completeWithTemplateParameters(const std::map<std::string, Datatype>& templateParams, const std::vector<std::string>& modules, Datatype::InternalCall internalCall, AllowIncompleteTypes allowIncompleteTypes) const {
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
        cpyFunctionType.first = ourFunctionType.first.completeWithTemplateParameters(templateParams, modules, internalCall, allowIncompleteTypes);
        size_t i = 0;
        for(auto& param : ourFunctionType.second) {
            cpyFunctionType.second.at(i) = param.completeWithTemplateParameters(templateParams, modules, internalCall, allowIncompleteTypes);
            ++i;
        }
        break;
    }
    case DatatypeCategory::undetermined_identifier: {
        auto maybeReplacementType = templateParams.end();
        for(auto& module : modules) {
            maybeReplacementType = templateParams.find(module + "." + getUndeterminedIdentifierString());
            if(maybeReplacementType != templateParams.end()) {
                break;
            }
        }
        // if we didn't find a replacement, try without prepending the module name
        if(maybeReplacementType == templateParams.end()) {
            maybeReplacementType = templateParams.find(getUndeterminedIdentifierString());
        }
        if(maybeReplacementType != templateParams.end()) {
            cpy = maybeReplacementType->second;
            if(cpy.getCategory() == DatatypeCategory::struct_ || cpy.getCategory() == DatatypeCategory::enum_) {
                // add template params to replacement map
                std::vector<Datatype> undeterminedIdentifierTemplateParameters;
                for(auto& dt : getUndeterminedIdentifierTemplateParams()) {
                    undeterminedIdentifierTemplateParameters.emplace_back(dt.completeWithTemplateParameters(templateParams, modules, internalCall, allowIncompleteTypes));
                }
                std::map<std::string, Datatype> additionalMap;
                if(!undeterminedIdentifierTemplateParameters.empty()) {
                    if(cpy.getCategory() == DatatypeCategory::struct_) {
                        additionalMap = createTemplateParamMap(cpy.getStructInfo().templateParams, undeterminedIdentifierTemplateParameters);
                    }  else {
                        additionalMap = createTemplateParamMap(cpy.getEnumInfo().templateParams, undeterminedIdentifierTemplateParameters);
                    }
                }
                additionalMap.insert(templateParams.cbegin(), templateParams.cend());
                cpy.attachUndeterminedIdentifierMap(std::make_shared<UndeterminedIdentifierCompletionInfo>(UndeterminedIdentifierCompletionInfo{ .map = additionalMap, .includedModules = modules }));
            }
            return cpy;
        }
        if(allowIncompleteTypes == AllowIncompleteTypes::No) {
            throw std::runtime_error{"Incomplete type not allowed here! You probably forgot to specify template parameters"};
        }
        break;
    }
    case DatatypeCategory::tuple: {
        for(size_t i = 0; i < getTupleInfo().size(); ++i) {
            std::get<std::vector<Datatype>>(*cpy.mFurtherInfo).at(i) = getTupleInfo().at(i).completeWithTemplateParameters(templateParams, modules, internalCall, allowIncompleteTypes);
        }
        break;
    }
    case DatatypeCategory::list: {
        std::get<Datatype>(*cpy.mFurtherInfo) = getListContainedType().completeWithTemplateParameters(templateParams, modules, internalCall, allowIncompleteTypes);
        break;
    }
    case DatatypeCategory::struct_: {
        size_t i = 0;
        for(auto& element : getStructInfo().fields) {
            std::get<StructInfo>(*cpy.mFurtherInfo).fields.at(i).type = getStructInfo().fields.at(i).type.completeWithTemplateParameters(templateParams, modules, internalCall, allowIncompleteTypes);
            ++i;
        }
        break;
    }
    case DatatypeCategory::enum_: {
        const auto& fields = getEnumInfo().fields;
        for(size_t i = 0; i < fields.size(); ++i) {
            const auto& fieldElements = fields.at(i).params;
            for(size_t j = 0; j < fieldElements.size(); ++j) {
                std::get<EnumInfo>(*mFurtherInfo).fields.at(i).params.at(j) = fieldElements.at(j).completeWithTemplateParameters(templateParams, modules, internalCall, allowIncompleteTypes);
            }
        }
        break;
    }
    default:
        assert(false);
    }
    cpy.attachUndeterminedIdentifierMap(std::make_shared<UndeterminedIdentifierCompletionInfo>(UndeterminedIdentifierCompletionInfo{ .map = templateParams, .includedModules = modules }));
    return cpy;
}
int32_t Datatype::EnumInfo::getLargestFieldSize(int32_t depth) const {
    int32_t largestFieldSize = 0;
    for(auto& field: fields) {
        int32_t fieldSize = 0;
        for(auto& element: field.params) {
            fieldSize += element.completeWithSavedTemplateParameters().getSizeOnStack(depth + 1);
        }
        largestFieldSize = std::max(fieldSize, largestFieldSize);
    }
    return largestFieldSize;
}
int32_t Datatype::EnumInfo::getLargestFieldSizePlusIndex(int32_t depth) const {
    return getLargestFieldSize(depth + 1) + getIndexSize();
}
int32_t Datatype::EnumInfo::getIndexSize() const {
#ifdef x86_64_BIT_MODE
    return 8;
#else
    return 4;
#endif
}
}