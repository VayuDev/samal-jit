#include <stdexcept>
#include "peg_parser/PegParser.hpp"
#include "peg_parser/PegTokenizer.hpp"
#include "peg_parser/PegParsingExpression.hpp"

namespace peg {

RuleResult PegParser::parse(const std::string_view &start, std::string code) {
  PegTokenizer tokenizer{std::move(code)};
  auto matchResult = mRules.at(std::string{start})->match(ParsingState{}, mRules, tokenizer);
  if(matchResult.index() == 0 && !tokenizer.isEmpty(std::get<0>(matchResult).first)) {
    // Characters left unconsumed
    // TODO return more info
    return ExpressionFailInfo{};
  }
  return matchResult;
}
void PegParser::addRule(std::string nonTerminal, std::shared_ptr<ParsingExpression> rule) {
  if (mRules.count(nonTerminal) > 0) {
    throw std::runtime_error{"A rule already exists for the Terminal " + nonTerminal};
  }
  mRules.emplace(std::make_pair(std::move(nonTerminal), std::move(rule)));
}

}
