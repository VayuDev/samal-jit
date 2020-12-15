#include <stdexcept>
#include "peg_parser/PegParser.hpp"
#include "peg_parser/PegTokenizer.hpp"
#include "peg_parser/PegParsingExpression.hpp"

namespace peg {

std::pair<std::variant<ExpressionSuccessInfo, ParsingFailInfo>, PegTokenizer> PegParser::parse(const std::string_view &start, std::string code) const {
  PegTokenizer tokenizer{std::move(code)};
  peg::NonTerminalParsingExpression fakeNonTerminal{std::string{start}};
  auto matchResult = fakeNonTerminal.match(ParsingState{}, mRules, tokenizer);
  if(matchResult.index() == 0 && !tokenizer.isEmpty(std::get<0>(matchResult).getState())) {
    // Characters left unconsumed
    return std::make_pair(ParsingFailInfo{
      .eof = true,
      .error = std::make_unique<ExpressionFailInfo>(std::get<0>(matchResult).moveFailInfo())
    }, std::move(tokenizer));
  }
  if(matchResult.index() == 1) {
    return std::make_pair(ParsingFailInfo{
        .eof = false,
        .error = std::make_unique<ExpressionFailInfo>(std::get<1>(matchResult))
    }, std::move(tokenizer));
  }
  return std::make_pair(std::variant<ExpressionSuccessInfo, ParsingFailInfo>(std::move(std::get<0>(matchResult))), std::move(tokenizer));
}
void PegParser::addRule(std::string nonTerminal, std::shared_ptr<ParsingExpression> rule, RuleCallback&& callback) {
  if (mRules.count(nonTerminal) > 0) {
    throw std::runtime_error{"A rule already exists for the Terminal " + nonTerminal};
  }
  mRules.emplace(std::make_pair(std::move(nonTerminal), Rule{  .expr = std::move(rule), .callback = std::move(callback) }));
}

Rule &PegParser::operator[](const char *non_terminal) {
  return mRules.emplace(non_terminal, Rule{}).first->second;
}

}
