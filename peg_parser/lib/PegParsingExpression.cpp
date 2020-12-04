#include "peg_parser/PegParsingExpression.hpp"

namespace peg {

SequenceParsingExpression::SequenceParsingExpression(std::vector<sp<ParsingExpression>> children)
    : mChildren(std::move(children)) {

}
RuleResult SequenceParsingExpression::match(ParsingState) const {
  return ExpressionFailInfo{};
}
std::string SequenceParsingExpression::dump() const {
  std::string ret;
  for (auto it = mChildren.cbegin(); it != mChildren.cend(); it++) {
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
  for (auto it = mChildren.cbegin(); it != mChildren.cend(); it++) {
    ret += (*it)->dump();
    if (std::next(it) != mChildren.cend()) {
      ret += " | ";
    }
  }
  ret += ')';
  return ret;
}
NonTerminalParsingExpression::NonTerminalParsingExpression(std::string value)
: mNonTerminal(std::move(value)) {

}
RuleResult NonTerminalParsingExpression::match(ParsingState) const {
  return ExpressionFailInfo{};
}
std::string NonTerminalParsingExpression::dump() const {
  return mNonTerminal;
}
OptionalParsingExpression::OptionalParsingExpression(sp<ParsingExpression> child)
: mChild(std::move(child)) {

}
RuleResult OptionalParsingExpression::match(ParsingState) const {
  return ExpressionFailInfo{};
}
std::string OptionalParsingExpression::dump() const {
  return "(" + mChild->dump() + ")?";
}
OneOrMoreParsingExpression::OneOrMoreParsingExpression(sp<ParsingExpression> child)
: mChild(std::move(child)) {

}
RuleResult OneOrMoreParsingExpression::match(ParsingState) const {
  return ExpressionFailInfo{};
}
std::string OneOrMoreParsingExpression::dump() const {
  return "(" + mChild->dump() + ")+";
}
ZeroOrMoreParsingExpression::ZeroOrMoreParsingExpression(sp<ParsingExpression> child)
: mChild(std::move(child)) {

}
RuleResult ZeroOrMoreParsingExpression::match(ParsingState) const {
  return ExpressionFailInfo{};
}
std::string ZeroOrMoreParsingExpression::dump() const {
  return "(" + mChild->dump() + ")*";
}
}