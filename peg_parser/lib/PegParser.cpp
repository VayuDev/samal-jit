#include <stdexcept>
#include "peg_parser/PegParser.hpp"
#include "peg_parser/PegTokenizer.hpp"

namespace peg {

std::any PegParser::parse(const std::string_view &start, std::string code) {
  PegTokenizer tokenizer{std::move(code)};

  return 0;
}
void PegParser::addRule(std::string nonTerminal, std::unique_ptr<ParsingExpression> rule) {
  if (mRules.count(nonTerminal) > 0) {
    throw std::runtime_error{"A rule already exists for the Terminal " + nonTerminal};
  }
  mRules.emplace(std::make_pair(std::move(nonTerminal), std::move(rule)));
}

}
