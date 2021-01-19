#pragma once
#include <cstdint>
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
static inline std::map<std::string, Datatype> createTemplateParamMap(const std::vector<Datatype>& identifierTemplateInfo /*<T>*/, const std::vector<Datatype>& templateParameterInfo /*<i32>*/) {
    std::map<std::string, Datatype> ret;
    size_t i = 0;
    for(auto& identifierTemplateParameter : identifierTemplateInfo) {
        ret.emplace(identifierTemplateParameter.getUndeterminedIdentifierInfo(), templateParameterInfo.at(i));
        ++i;
    }
    return ret;
};

}