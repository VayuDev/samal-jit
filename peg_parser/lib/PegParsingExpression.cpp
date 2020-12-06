#include <cassert>
#include "peg_parser/PegParsingExpression.hpp"
#include "peg_parser/PegTokenizer.hpp"

namespace peg {

SequenceParsingExpression::SequenceParsingExpression(std::vector<sp<ParsingExpression>> children)
    : mChildren(std::move(children)) {

}
RuleResult SequenceParsingExpression::match(ParsingState state, const RuleMap& rules, const PegTokenizer& tokenizer) const {
  auto start = tokenizer.getPtr(state);
  std::vector<MatchInfo> childrenResults;
  std::vector<ExpressionFailInfo> childrenFailReasons;
  for(auto& child: mChildren) {
    auto childRet = child->match(state, rules, tokenizer);
    if(childRet.index() == 0) {
      // child matched
      state = std::get<0>(childRet).getState();
      childrenResults.emplace_back(std::get<0>(childRet).moveMatchInfo());
      childrenFailReasons.emplace_back(std::get<0>(childRet).moveFailInfo());
    } else {
      // child failed
      childrenFailReasons.emplace_back(std::move(std::get<1>(childRet)));
      return ExpressionFailInfo {
        state,
        ExpressionFailReason::SEQUENCE_CHILD_FAILED,
        std::string{std::string_view(start, tokenizer.getPtr(state) - start)},
        std::move(childrenFailReasons)
      };
    }
  }
  return  ExpressionSuccessInfo{
    state,
    MatchInfo{
      .start = start,
      .end = tokenizer.getPtr(state),
      .subs = std::move(childrenResults)
    },
    ExpressionFailInfo{state, std::move(childrenFailReasons)}
  };
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
  std::optional<ParsingState> tokenizerRet;
  if(mRegex) {
    tokenizerRet = tokenizer.matchRegex(state, *mRegex);
  } else {
    tokenizerRet = tokenizer.matchString(state, mStringRepresentation);
  }
  if(tokenizerRet) {
    return ExpressionSuccessInfo{
        *tokenizerRet,
        MatchInfo{
            .start = start,
            .end = tokenizer.getPtr(*tokenizerRet)
        },
        ExpressionFailInfo{state}
    };
  } else {
    // TODO cache regex
    size_t len = tokenizer.getPtr(*tokenizer.matchRegex(state, std::regex{"[^ ]*"})) - tokenizer.getPtr(state);
    std::string failedString{std::string_view{tokenizer.getPtr(state), len}};
    return ExpressionFailInfo{
        state,
        ExpressionFailReason::UNMATCHED_STRING,
        mStringRepresentation,
        failedString
    };
  }
}
std::string TerminalParsingExpression::dump() const {
  if(mRegex) {
    return mStringRepresentation;
  }
  return '\'' + mStringRepresentation + '\'';
}
ChoiceParsingExpression::ChoiceParsingExpression(std::vector<sp<ParsingExpression>> children)
    : mChildren(std::move(children)) {

}
RuleResult ChoiceParsingExpression::match(ParsingState state, const RuleMap& rules, const PegTokenizer& tokenizer) const {
  std::vector<ExpressionFailInfo> childrenFailReasons;
  size_t i = 0;
  for(auto& child: mChildren) {
    auto childRet = child->match(state, rules, tokenizer);
    if(childRet.index() == 0) {
      // child matched
      return ExpressionSuccessInfo{
          std::get<0>(childRet).getState(),
          MatchInfo{
              .start = std::get<0>(childRet).getMatchInfo().start,
              .end = std::get<0>(childRet).getMatchInfo().end,
              .choice = i,
              .subs = {std::get<0>(childRet).moveMatchInfo()}},
          ExpressionFailInfo{state, std::move(childrenFailReasons)}};
    } else {
      childrenFailReasons.emplace_back(std::get<1>(childRet));
    }
    i++;
  }
  // TODO return more info
  return ExpressionFailInfo {
    state,
    ExpressionFailReason::CHOICE_NO_CHILD_SUCCEEDED,
    std::move(childrenFailReasons)};
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
  std::any callbackResult;
  if(rule.callback) {
    callbackResult = rule.callback(std::get<0>(ruleRetValue).getMatchInfo());
  }
  auto end = tokenizer.getPtr(state);
  return ExpressionSuccessInfo{std::get<0>(ruleRetValue).getState(), MatchInfo{
    .start = start,
    .end = end,
    .result = std::move(callbackResult),
    .subs = { std::get<0>(ruleRetValue).moveMatchInfo() }
  },ExpressionFailInfo{std::get<0>(ruleRetValue).getState(), {std::get<0>(ruleRetValue).moveFailInfo()}}};
}
std::string NonTerminalParsingExpression::dump() const {
  return mNonTerminal;
}
OptionalParsingExpression::OptionalParsingExpression(sp<ParsingExpression> child)
: mChild(std::move(child)) {

}
RuleResult OptionalParsingExpression::match(ParsingState state, const RuleMap& rules, const PegTokenizer& tokenizer) const {
  auto start = tokenizer.getPtr(state);
  auto childRet = mChild->match(state, rules, tokenizer);
  if(childRet.index() == 1) {
    // error
    return ExpressionSuccessInfo{state, MatchInfo{
      .start = start,
      .end = start
    },ExpressionFailInfo{state, { std::move(std::get<1>(childRet)) }}};
  }
  // success
  state = std::get<0>(childRet).getState();
  return ExpressionSuccessInfo{state, MatchInfo{
        .start = start,
        .end = tokenizer.getPtr(state),
        .subs = { std::get<0>(childRet).moveMatchInfo() }
  },ExpressionFailInfo{state, { std::get<0>(childRet).moveFailInfo() }}};

  assert(false);
}
std::string OptionalParsingExpression::dump() const {
  return "(" + mChild->dump() + ")?";
}
OneOrMoreParsingExpression::OneOrMoreParsingExpression(sp<ParsingExpression> child)
: mChild(std::move(child)) {

}
RuleResult OneOrMoreParsingExpression::match(ParsingState state, const RuleMap& rules, const PegTokenizer& tokenizer) const {
  auto start = tokenizer.getPtr(state);
  auto childRet = mChild->match(state, rules, tokenizer);
  if(childRet.index() == 1) {
    // error
    return ExpressionFailInfo{state, ExpressionFailReason::REQUIRED_ONE_OR_MORE, {std::get<1>(childRet)}};
  }
  // success
  state = std::get<0>(childRet).getState();
  std::vector<MatchInfo> childrenResults;
  std::vector<ExpressionFailInfo> childrenFailReasons;
  childrenResults.emplace_back(std::get<0>(childRet).moveMatchInfo());
  while(true) {
    childRet = mChild->match(state, rules, tokenizer);
    if(childRet.index() == 1) {
      childrenFailReasons.emplace_back(std::move(std::get<1>(childRet)));
      break;
    }
    childrenResults.emplace_back(std::get<0>(childRet).moveMatchInfo());
    childrenFailReasons.emplace_back(std::get<0>(childRet).moveFailInfo());
    state = std::get<0>(childRet).getState();
  }
  return ExpressionSuccessInfo{state, MatchInfo{
    .start = start,
    .end = tokenizer.getPtr(state),
    .subs = std::move(childrenResults)
  }, ExpressionFailInfo{state, std::move(childrenFailReasons)}};
}
std::string OneOrMoreParsingExpression::dump() const {
  return "(" + mChild->dump() + ")+";
}
ZeroOrMoreParsingExpression::ZeroOrMoreParsingExpression(sp<ParsingExpression> child)
: mChild(std::move(child)) {

}
RuleResult ZeroOrMoreParsingExpression::match(ParsingState state, const RuleMap& rules, const PegTokenizer& tokenizer) const {
  auto start = tokenizer.getPtr(state);
  std::vector<MatchInfo> childrenResults;
  std::vector<ExpressionFailInfo> childrenFailReasons;
  while(true) {
    auto childRet = mChild->match(state, rules, tokenizer);
    if(childRet.index() == 1) {
      childrenFailReasons.emplace_back(std::move(std::get<1>(childRet)));
      break;
    }
    childrenResults.emplace_back(std::get<0>(childRet).moveMatchInfo());
    childrenFailReasons.emplace_back(std::get<0>(childRet).moveFailInfo());
    state = std::get<0>(childRet).getState();
  }
  return ExpressionSuccessInfo{state, MatchInfo{
      .start = start,
      .end = tokenizer.getPtr(state),
      .subs = std::move(childrenResults)
  }, ExpressionFailInfo{state, std::move(childrenFailReasons)}};
}
std::string ZeroOrMoreParsingExpression::dump() const {
  return "(" + mChild->dump() + ")*";
}
std::string errorsToStringRec(const ExpressionFailInfo & info, const PegTokenizer& tokenizer, int depth) {
  std::string ret;
  for(int i = 0; i < depth; ++i) {
    ret += " ";
  }
  ret += info.dump(tokenizer) + "\n";
  for(const auto& child: info.mSubExprFailInfo) {
    ret += errorsToStringRec(child, tokenizer, depth + 1);
  }
  return ret;
}
std::string errorsToString(const ExpressionFailInfo & info, const PegTokenizer& tokenizer) {
  return errorsToStringRec(info, tokenizer, 0);
}
}