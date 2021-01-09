#pragma once

#include <any>
#include <map>
#include <memory>
#include <regex>
#include <string>
#include <string_view>
#include <tuple>
#include <variant>
#include <vector>

#include "peg_parser/PegParsingExpression.hpp"
#include "peg_parser/PegTokenizer.hpp"
#include "peg_parser/PegUtil.hpp"

namespace peg {

using ParserResult = std::pair<std::variant<ExpressionSuccessInfo, ParsingFailInfo>, PegTokenizer>;

class PegParser {
public:
    void addRule(std::string nonTerminal, std::shared_ptr<ParsingExpression> rule, RuleCallback&& callback = {});
    ParserResult parse(const std::string_view& start, std::string code) const;
    Rule& operator[](const char* non_terminal);

private:
    RuleMap mRules;
};

}