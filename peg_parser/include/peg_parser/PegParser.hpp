#pragma once

#include <string>
#include <any>
#include <string_view>
#include <map>
#include <variant>
#include <memory>
#include <vector>
#include <regex>
#include <tuple>

#include "peg_parser/PegTokenizer.hpp"
#include "peg_parser/PegParsingExpression.hpp"
#include "peg_parser/PegUtil.hpp"

namespace peg {

class PegParser {
 public:
  void addRule(std::string nonTerminal, std::shared_ptr<ParsingExpression> rule, RuleCallback&& callback = {});
  std::pair<std::variant<ExpressionSuccessInfo, ParsingFailInfo>, PegTokenizer> parse(const std::string_view& start, std::string code) const;
  Rule& operator[](const char *non_terminal);
 private:
  RuleMap mRules;
};

}