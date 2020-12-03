#include <stdexcept>
#include "peg_parser/PegParser.hpp"
#include "peg_parser/PegTokenizer.hpp"

namespace peg {

std::any PegParser::parse(const std::string_view &start, std::string code) {
  PegTokenizer tokenizer{std::move(code)};


  return 0;
}
void PegParser::addRule(std::string nonTerminal, std::unique_ptr<ParsingExpression> rule) {
  if(mRules.count(nonTerminal) > 0) {
    throw std::runtime_error{"A rule already exists for the Terminal " + nonTerminal};
  }
  mRules.emplace(std::make_pair(std::move(nonTerminal), std::move(rule)));
}

SequenceParsingExpression::SequenceParsingExpression(std::vector<sp<ParsingExpression>> children)
: mChildren(std::move(children)) {

}
RuleResult SequenceParsingExpression::match(ParsingState) const {
  return ExpressionFailInfo{};
}
std::string SequenceParsingExpression::dump() const {
  std::string ret;
  for(auto it = mChildren.cbegin(); it != mChildren.cend(); it++) {
    ret += (*it)->dump();
    if (std::next(it) != mChildren.cend()) {
      ret += " ";
    }
  }
  return ret;
}

TerminalParsingExpression::TerminalParsingExpression(std::string value)
: mStringRepresentation(std::move(value)) {

}
TerminalParsingExpression::TerminalParsingExpression(std::string stringRep, std::regex value)
: mStringRepresentation(std::move(stringRep)), mRegex(std::move(value)) {

}
RuleResult TerminalParsingExpression::match(ParsingState state) const {
  return ExpressionFailInfo{};
}
std::string TerminalParsingExpression::dump() const {
  return '\'' + mStringRepresentation + '\'';
}
ChoiceParsingExpression::ChoiceParsingExpression(std::vector<sp<ParsingExpression>> children)
: mChildren(std::move(children)) {

}
RuleResult ChoiceParsingExpression::match(ParsingState) const {
  return ExpressionFailInfo{};
}
std::string ChoiceParsingExpression::dump() const {
  std::string ret = "(";
  for(auto it = mChildren.cbegin(); it != mChildren.cend(); it++) {
    ret += (*it)->dump();
    if (std::next(it) != mChildren.cend()) {
      ret += " | ";
    }
  }
  ret += ')';
  return ret;
}
}
