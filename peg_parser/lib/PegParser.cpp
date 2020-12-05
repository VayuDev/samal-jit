#include <stdexcept>
#include "peg_parser/PegParser.hpp"
#include "peg_parser/PegTokenizer.hpp"
#include "peg_parser/PegParsingExpression.hpp"

namespace peg {

RuleResult PegParser::parse(const std::string_view &start, std::string code) {
  PegTokenizer tokenizer{std::move(code)};
  peg::NonTerminalParsingExpression fakeNonTerminal{std::string{start}};
  auto matchResult = fakeNonTerminal.match(ParsingState{}, mRules, tokenizer);
  if(matchResult.index() == 0 && !tokenizer.isEmpty(std::get<0>(matchResult).first)) {
    // Characters left unconsumed
    // TODO return more info
    return ExpressionFailInfo{};
  }
  return matchResult;
}
void PegParser::addRule(std::string nonTerminal, std::shared_ptr<ParsingExpression> rule, RuleCallback&& callback) {
  if (mRules.count(nonTerminal) > 0) {
    throw std::runtime_error{"A rule already exists for the Terminal " + nonTerminal};
  }
  mRules.emplace(std::make_pair(std::move(nonTerminal), Rule{  .expr = std::move(rule), .callback = std::move(callback) }));
}

}
