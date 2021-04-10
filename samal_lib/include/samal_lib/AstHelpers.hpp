#pragma once
#include <cstdint>
#include <stdexcept>
#include "Forward.hpp"
#include "Util.hpp"
#include <cstdint>
#include <map>
#include <stdexcept>
#include "Datatype.hpp"

namespace samal {

struct IdentifierId {
    int32_t variableId = 0;
    int32_t templateId = 0;
};

static inline bool operator<(const IdentifierId& a, const IdentifierId& b) {
    if(a.variableId < b.variableId)
        return true;
    if(a.templateId < b.templateId)
        return true;
    return false;
}

using TemplateInstantiationInfo = std::vector<std::vector<Datatype>>;
// create a map that maps e.g. T => i32 if you call fib<i32>,
// which is then used to determine the return & param types of fib<i32>
// which could be fib<T> -> T, so it becomes i32
static inline UndeterminedIdentifierReplacementMap createTemplateParamMap(const std::vector<std::string>& identifierTemplateInfo /*<T>*/, const std::vector<Datatype>& templateParameterInfo /*<i32>*/, const std::vector<std::string>& usingModuleNames) {
    if(identifierTemplateInfo.size() != templateParameterInfo.size()) {
        throw std::runtime_error{ "Template parameter sizes don't match" };
    }
    UndeterminedIdentifierReplacementMap ret;
    size_t i = 0;
    for(auto& identifierTemplateParameter : identifierTemplateInfo) {
        if(i >= templateParameterInfo.size()) {
            return ret;
        }
        ret.emplace(identifierTemplateParameter, UndeterminedIdentifierReplacementMapValue{templateParameterInfo.at(i), TemplateParamOrUserType::TemplateParam, usingModuleNames});
        ++i;
    }
    return ret;
};

}