#include "peg_parser/PegParsingExpression.hpp"
#include "peg_parser/PegTokenizer.hpp"

namespace peg {

SequenceParsingExpression::SequenceParsingExpression(std::vector<sp<ParsingExpression>> children)
    : mChildren(std::move(children)) {

}
RuleResult SequenceParsingExpression::match(ParsingState state, const RuleMap& rules, const PegTokenizer& tokenizer) const {
  auto start = tokenizer.getPtr(state);
  std::vector<MatchInfo> childrenResults;
  for(auto& child: mChildren) {
    auto childRet = child->match(state, rules, tokenizer);
    if(childRet.index() == 0) {
      // child matched
      state = std::get<0>(childRet).first;
      childrenResults.emplace_back(std::move(std::get<0>(childRet).second));
    } else {
      // child failed
      // TODO return more information
      return ExpressionFailInfo{};
    }
  }
  return std::make_pair(state, MatchInfo{
    .start = start,
    .end = tokenizer.getPtr(state),
    .subs = std::move(childrenResults)
  });
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
RuleResult TerminalParsingExpression::match(ParsingState state, const RuleMap&, const PegTokenizer& tokenizer) const {
  auto start = tokenizer.getPtr(state);
  if(mRegex) {
    auto newState = tokenizer.matchRegex(state, *mRegex);
    if(newState) {
      return std::make_pair(*newState, MatchInfo{
        .start = start,
        .end = tokenizer.getPtr(*newState)
      });
    }
  } else {
    auto newState = tokenizer.matchString(state, mStringRepresentation);
    if(newState) {
      return std::make_pair(*newState, MatchInfo{
          .start = start,
          .end = tokenizer.getPtr(*newState)
      });
    }
  }
  // TODO return more information
  return ExpressionFailInfo{};
}
std::string TerminalParsingExpression::dump() const {
  return '\'' + mStringRepresentation + '\'';
}
ChoiceParsingExpression::ChoiceParsingExpression(std::vector<sp<ParsingExpression>> children)
    : mChildren(std::move(children)) {

}
RuleResult ChoiceParsingExpression::match(ParsingState state, const RuleMap& rules, const PegTokenizer& tokenizer) const {
  size_t i = 0;
  for(auto& child: mChildren) {
    auto childRet = child->match(state, rules, tokenizer);
    if(childRet.index() == 0) {
      // child matched
      return std::make_pair(std::get<0>(childRet).first, MatchInfo{
        .start = std::get<0>(childRet).second.start,
        .end = std::get<0>(childRet).second.end,
        .choice = i,
        .subs = { std::get<0>(childRet).second }});
    }
    i++;
  }
  // TODO return more info
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
RuleResult NonTerminalParsingExpression::match(ParsingState state, const RuleMap& rules, const PegTokenizer& tokenizer) const {
  auto start = tokenizer.getPtr(state);
  auto& rule = rules.at(mNonTerminal);
  auto ruleRetValue = rule.expr->match(state, rules, tokenizer);
  if(ruleRetValue.index() == 1) {
    // error in child
    return ruleRetValue;
  }
  auto end = tokenizer.getPtr(state);
  return std::make_pair(std::get<0>(ruleRetValue).first, MatchInfo{
    .start = start,
    .end = end,
    .subs = { std::move(std::get<0>(ruleRetValue).second) }
  });
}
std::string NonTerminalParsingExpression::dump() const {
  return mNonTerminal;
}
OptionalParsingExpression::OptionalParsingExpression(sp<ParsingExpression> child)
: mChild(std::move(child)) {

}
RuleResult OptionalParsingExpression::match(ParsingState, const RuleMap&, const PegTokenizer& tokenizer) const {
  return ExpressionFailInfo{};
}
std::string OptionalParsingExpression::dump() const {
  return "(" + mChild->dump() + ")?";
}
OneOrMoreParsingExpression::OneOrMoreParsingExpression(sp<ParsingExpression> child)
: mChild(std::move(child)) {

}
RuleResult OneOrMoreParsingExpression::match(ParsingState, const RuleMap&, const PegTokenizer& tokenizer) const {
  return ExpressionFailInfo{};
}
std::string OneOrMoreParsingExpression::dump() const {
  return "(" + mChild->dump() + ")+";
}
ZeroOrMoreParsingExpression::ZeroOrMoreParsingExpression(sp<ParsingExpression> child)
: mChild(std::move(child)) {

}
RuleResult ZeroOrMoreParsingExpression::match(ParsingState, const RuleMap&, const PegTokenizer& tokenizer) const {
  return ExpressionFailInfo{};
}
std::string ZeroOrMoreParsingExpression::dump() const {
  return "(" + mChild->dump() + ")*";
}
}