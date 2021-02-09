#include "samal_lib/Datatype.hpp"
#include <cassert>
#include "samal_lib/AST.hpp"

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
        ret += ") -> " + functionInfo.first->toString();
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
        ;
    }
    case DatatypeCategory::bool_:
        return "bool";
    case DatatypeCategory::undetermined_identifier:
        return "<undetermined '" + std::get<std::string>(mFurtherInfo) + "'>";
    case DatatypeCategory::struct_:
        return "<struct " + std::get<StructInfo>(mFurtherInfo).name + ">";
    default:
        return "DATATYPE";
    }
}
Datatype::Datatype()
: mCategory(DatatypeCategory::invalid) {
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
const std::pair<sp<Datatype>, std::vector<Datatype>>& Datatype::getFunctionTypeInfo() const& {
    if(mCategory != DatatypeCategory::function) {
        throw std::runtime_error{ "Can't call this type!" };
    }
    return std::get<std::pair<sp<Datatype>, std::vector<Datatype>>>(mFurtherInfo);
}
const std::vector<Datatype>& Datatype::getTupleInfo() const {
    if(mCategory != DatatypeCategory::tuple) {
        throw std::runtime_error{ "This is not a tuple!" };
    }
    return std::get<std::vector<Datatype>>(mFurtherInfo);
}
const Datatype& Datatype::getListInfo() const {
    if(mCategory != DatatypeCategory::list) {
        throw std::runtime_error{ "This is not a list!" };
    }
    return *std::get<sp<Datatype>>(mFurtherInfo);
}
const std::string& Datatype::getUndeterminedIdentifierInfo() const {
    if(mCategory != DatatypeCategory::undetermined_identifier) {
        throw std::runtime_error{ "This is not an undetermined identifier!" };
    }
    return std::get<std::string>(mFurtherInfo);
}
const Datatype::StructInfo& Datatype::getStructInfo() const {
    if(mCategory != DatatypeCategory::struct_) {
        throw std::runtime_error{ "This is not a struct!" };
    }
    return std::get<StructInfo>(mFurtherInfo);
}
bool Datatype::operator==(const Datatype& other) const {
    if(mCategory != other.mCategory)
        return false;
    if(mFurtherInfo.index() != other.mFurtherInfo.index())
        return false;
    try {
        // check for function type equality
        auto& selfValue = std::get<std::pair<sp<Datatype>, std::vector<Datatype>>>(mFurtherInfo);
        auto& otherValue = std::get<std::pair<sp<Datatype>, std::vector<Datatype>>>(other.mFurtherInfo);
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
        try {
            // check for array type equality
            auto& selfValue = std::get<sp<Datatype>>(mFurtherInfo);
            auto& otherValue = std::get<sp<Datatype>>(other.mFurtherInfo);
            if(selfValue != otherValue) {
                if(selfValue && otherValue) {
                    if(*selfValue != *otherValue) {
                        return false;
                    } else {
                        return true;
                    }
                }
            }
        } catch(std::bad_variant_access&) {
            // fall back to simple check
            if(mFurtherInfo != other.mFurtherInfo) {
                return false;
            }
        }
    }
    return true;
}
bool Datatype::operator!=(const Datatype& other) const {
    return !(*this == other);
}
DatatypeCategory Datatype::getCategory() const {
    return mCategory;
}
Datatype Datatype::createEmptyTuple() {
    return Datatype(std::vector<Datatype>{});
}
Datatype Datatype::createListType(Datatype baseType) {
    Datatype ret{ DatatypeCategory::list };
    ret.mFurtherInfo = std::make_shared<Datatype>(std::move(baseType));
    return ret;
}
Datatype Datatype::createStructType(const std::string& name, const std::vector<Parameter>& params, const std::vector<Datatype>& templateParams) {
    std::vector<StructInfo::StructElement> elements;
    for(const auto& p: params) {
        elements.push_back(StructInfo::StructElement{.name = p.name->getName(), .baseType = p.type});
    }
    std::sort(elements.begin(), elements.end(), [](auto& a, auto& b) -> bool {
        return a.name < b.name;
    });
    std::vector<std::string> templateParamNames;
    for(auto& t: templateParams) {
        assert(t.getCategory() == DatatypeCategory::undetermined_identifier);
        templateParamNames.push_back(t.getUndeterminedIdentifierInfo());
    }
    return Datatype(DatatypeCategory::struct_, StructInfo{.name = name, .elements = std::move(elements), .templateParams = std::move(templateParamNames)});
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
    Datatype cpy = *this;
    switch(mCategory) {
    case DatatypeCategory::bool_:
    case DatatypeCategory::i32:
    case DatatypeCategory::i64:
    case DatatypeCategory::string:
        break;
    case DatatypeCategory::function: {
        auto functionType = getFunctionTypeInfo();
        std::get<std::pair<sp<Datatype>, std::vector<Datatype>>>(cpy.mFurtherInfo).first = std::make_shared<Datatype>(functionType.first->completeWithTemplateParameters(templateParams));
        size_t i = 0;
        for(auto& param : functionType.second) {
            std::get<std::pair<sp<Datatype>, std::vector<Datatype>>>(cpy.mFurtherInfo).second[i] = param.completeWithTemplateParameters(templateParams);
            ++i;
        }
        break;
    }
    case DatatypeCategory::undetermined_identifier: {
        auto maybeReplacementType = templateParams.find(std::get<std::string>(mFurtherInfo));
        if(maybeReplacementType != templateParams.end()) {
            cpy = maybeReplacementType->second;
        }
        // recursively complete types if the type we replaced our copy with requires template parameters as well; only possible right now if it's a struct
        if(cpy.getCategory() == DatatypeCategory::struct_) {
            return cpy.completeWithTemplateParameters(templateParams);
        }
        break;
    }
    case DatatypeCategory::tuple: {
        for(size_t i = 0; i < getTupleInfo().size(); ++i) {
            std::get<std::vector<Datatype>>(cpy.mFurtherInfo).at(i) = std::get<std::vector<Datatype>>(mFurtherInfo).at(i).completeWithTemplateParameters(templateParams);
        }
        break;
    }
    case DatatypeCategory::list: {
        std::get<sp<Datatype>>(cpy.mFurtherInfo) = std::make_shared<Datatype>(getListInfo().completeWithTemplateParameters(templateParams));
        break;
    }
    case DatatypeCategory::struct_: {
        size_t i = 0;
        auto templateParamsCpy = templateParams;

        for(auto& element : getStructInfo().elements) {
            std::get<StructInfo>(cpy.mFurtherInfo).elements.at(i).lazyType = [baseType = getStructInfo().elements.at(i).baseType, templateParams = templateParamsCpy] () {
                return baseType.completeWithTemplateParameters(templateParams);
            };
            ++i;
        }
        break;
    }
    default:
        assert(false);
    }
    return cpy;
}
bool Datatype::hasUndeterminedTemplateTypes() const {
    switch(mCategory) {
    case DatatypeCategory::bool_:
    case DatatypeCategory::i32:
    case DatatypeCategory::i64:
    case DatatypeCategory::string:
        return false;
    case DatatypeCategory::function:
        if(getFunctionTypeInfo().first->hasUndeterminedTemplateTypes())
            return true;
        for(auto& param : getFunctionTypeInfo().second) {
            if(param.hasUndeterminedTemplateTypes())
                return true;
        }
        return false;
    case DatatypeCategory::undetermined_identifier: {
        return true;
    }
    case DatatypeCategory::tuple: {
        for(auto& child : getTupleInfo()) {
            if(child.hasUndeterminedTemplateTypes())
                return true;
        }
        return false;
    }
    case DatatypeCategory::list: {
        return getListInfo().hasUndeterminedTemplateTypes();
    }
    case DatatypeCategory::struct_: {
        for(auto& element : getStructInfo().elements) {
            if(!element.lazyType)
                return true;
        }
        return false;
    }
    default:
        assert(false);
    }
}
Datatype::Datatype(DatatypeCategory cat, decltype(mFurtherInfo) furtherInfo)
: mFurtherInfo(std::move(furtherInfo)), mCategory(cat) {
}

}