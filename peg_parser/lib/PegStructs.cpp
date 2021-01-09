#include "peg_parser/PegStructs.hpp"
#include "peg_parser/PegParsingExpressionParser.hpp"

namespace peg {

Rule& Rule::operator<<(const char* ruleString) {
    this->expr = stringToParsingExpression(ruleString);
    return *this;
}
Rule& Rule::operator>>(RuleCallback&& ruleCallback) {
    callback = std::move(ruleCallback);
    return *this;
}

}