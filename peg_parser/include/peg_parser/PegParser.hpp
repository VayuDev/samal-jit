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
  void addRule(std::string nonTerminal, std::unique_ptr<ParsingExpression> rule);
  std::any parse(const std::string_view& start, std::string code);
 private:
  std::map<std::string, up<ParsingExpression>> mRules;
};

}