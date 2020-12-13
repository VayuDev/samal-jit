#include <cassert>
#include "peg_parser/PegParsingExpression.hpp"
#include "peg_parser/PegTokenizer.hpp"

namespace peg {

std::string ExpressionFailInfo::dump(const PegTokenizer& tokenizer, bool reverseOrder) const {
  auto[line, column] = tokenizer.getPosition(mState);
  std::string msg;
  switch(mReason) {
    case ExpressionFailReason::SEQUENCE_CHILD_FAILED:
      msg += "\033[36m";
      break;
    case ExpressionFailReason::CHOICE_NO_CHILD_SUCCEEDED:
      msg += "\033[34m";
      break;
    case ExpressionFailReason::ADDITIONAL_ERROR_MESSAGE:
      msg += "\033[31m";
      break;
    default:
      break;
  }
  msg += "" + std::to_string(line) + ":" + std::to_string(column) + ": " + mSelfDump + " - ";
  if(!reverseOrder) {
    switch (mReason) {
      case ExpressionFailReason::SUCCESS:
        msg += "Success!";
        break;
      case ExpressionFailReason::SEQUENCE_CHILD_FAILED:
        msg += "Needed to parse more children - already parsed: '" + mInfo1 + "'. Children failure info:\033[0m";
        break;
      case ExpressionFailReason::CHOICE_NO_CHILD_SUCCEEDED:
        msg += "No possible choice matched. The options were (in order):\033[0m";
        break;
      case ExpressionFailReason::REQUIRED_ONE_OR_MORE:
        msg += "We need to match one or more of the following, but not even one succeeded:";
        break;
      case ExpressionFailReason::UNMATCHED_REGEX:msg += "The regex '" + mInfo1 + "' didn't match '" + mInfo2 + "'";
        break;
      case ExpressionFailReason::UNMATCHED_STRING:msg += "The string '" + mInfo1 + "' didn't match '" + mInfo2 + "'";
        break;
      case ExpressionFailReason::ADDITIONAL_ERROR_MESSAGE:
        msg += mInfo1 + ", got: '" + mInfo2 + "'\033[0m";
        break;
      default:assert(false);
    }
  } else {
    switch (mReason) {
      case ExpressionFailReason::SUCCESS:
        msg += "Success!";
        break;
      case ExpressionFailReason::SEQUENCE_CHILD_FAILED:
        msg += "Needed to parse more children - already parsed: '" + mInfo1 + "'. Children failure info:\033[0m";
        break;
      case ExpressionFailReason::CHOICE_NO_CHILD_SUCCEEDED:
        msg += "No child option succeeded.\033[0m";
        break;
      case ExpressionFailReason::REQUIRED_ONE_OR_MORE:
        msg += "We need to match one or more of the following, but not even one succeeded:";
        break;
      case ExpressionFailReason::UNMATCHED_REGEX:msg += "The regex '" + mInfo1 + "' didn't match '" + mInfo2 + "'";
        break;
      case ExpressionFailReason::UNMATCHED_STRING:msg += "The string '" + mInfo1 + "' didn't match '" + mInfo2 + "'";
        break;
      case ExpressionFailReason::ADDITIONAL_ERROR_MESSAGE:
        msg += mInfo1 + " (" + mInfo2 + ")\033[0m";
        break;
      default:assert(false);
    }
  }

  return msg;
}

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
        dump(),
        ExpressionFailReason::SEQUENCE_CHILD_FAILED,
        std::string{std::string_view(start, tokenizer.getPtr(state) - start)},
        "",
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
    ExpressionFailInfo{state, dump(), std::move(childrenFailReasons)}
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
        ExpressionFailInfo{state, dump()}
    };
  } else {
    // TODO cache regex
    size_t len = tokenizer.getPtr(*tokenizer.matchRegex(state, std::regex{"^[^ ]*"})) - tokenizer.getPtr(state);
    std::string failedString{std::string_view{tokenizer.getPtr(state), len}};
    return ExpressionFailInfo{
        state,
        dump(),
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
          ExpressionFailInfo{state, dump(), std::move(childrenFailReasons)}};
    } else {
      childrenFailReasons.emplace_back(std::get<1>(childRet));
    }
    i++;
  }
  // TODO return more info
  return ExpressionFailInfo {
    state,
    dump(),
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
  }, std::get<0>(ruleRetValue).moveFailInfo()};
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
    },ExpressionFailInfo{state, dump(), { std::move(std::get<1>(childRet)) }}};
  }
  // success
  state = std::get<0>(childRet).getState();
  return ExpressionSuccessInfo{state, MatchInfo{
        .start = start,
        .end = tokenizer.getPtr(state),
        .subs = { std::get<0>(childRet).moveMatchInfo() }
  },ExpressionFailInfo{state, dump(), { std::get<0>(childRet).moveFailInfo() }}};

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
    return ExpressionFailInfo{state, dump(), ExpressionFailReason::REQUIRED_ONE_OR_MORE, {std::get<1>(childRet)}};
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
  }, ExpressionFailInfo{state, dump(), std::move(childrenFailReasons)}};
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
  }, ExpressionFailInfo{state, dump(), std::move(childrenFailReasons)}};
}
std::string ZeroOrMoreParsingExpression::dump() const {
  return "(" + mChild->dump() + ")*";
}
ErrorMessageInfoExpression::ErrorMessageInfoExpression(sp<ParsingExpression> child, std::string error)
: mChild(std::move(child)), mErrorMsg(std::move(error)) {

}
RuleResult ErrorMessageInfoExpression::match(ParsingState state, const RuleMap &rules, const PegTokenizer &tokenizer) const {
  auto childRes = mChild->match(state, rules, tokenizer);
  if(childRes.index() == 0) {
    return childRes;
  }
  // TODO cache regex
  size_t len = tokenizer.getPtr(*tokenizer.matchRegex(state, std::regex{"^[^\\s]*"})) - tokenizer.getPtr(state);
  std::string failedString{std::string_view{tokenizer.getPtr(state), len}};
  return ExpressionFailInfo{
    state,
    mChild->dump(),
    ExpressionFailReason::ADDITIONAL_ERROR_MESSAGE,
    mErrorMsg,
    std::move(failedString),
    { std::move(std::get<1>(childRes)) }
  };
}
std::string ErrorMessageInfoExpression::dump() const {
  return mChild->dump() + " #" + mErrorMsg + "#";
}

class ErrorTree {
 public:
  ErrorTree(wp<ErrorTree> parent, const ExpressionFailInfo& sourceNode)
  : mParent(std::move(parent)), mSourceNode(sourceNode) {

  }
  void attach(sp<ErrorTree>&& child) {
    mChildren.emplace_back(std::move(child));
  }
  [[nodiscard]] const auto& getChildren() const {
    return mChildren;
  }
  [[nodiscard]] auto getParent() const {
    return mParent;
  }
  [[nodiscard]] const auto& getSourceNode() const {
    return mSourceNode;
  }
  [[nodiscard]] std::string dump(const PegTokenizer& tok, size_t depth) const {
    std::string ret = dumpSelf(tok, depth, false);
    if(!mSourceNode.isAdditionalErrorMessage()) {
      for(const auto& child: mChildren) {
        ret += child->dump(tok, depth + 1);
      }
    }
    return ret;
  }
  [[nodiscard]] std::string dumpReverse(const PegTokenizer& tok, size_t depth, size_t maxDepth) const {
    if(depth >= maxDepth) {
      return "";
    }
    std::string ret;
    for(const auto& child: mChildren) {
      assert(depth > 0);
      ret += child->dumpSelf(tok, depth - 1, true);
    }
    ret += mParent.lock()->dumpReverse(tok, depth + 1, maxDepth);
    return ret;
  }
 private:
  [[nodiscard]] std::string dumpSelf(const PegTokenizer& tok, size_t depth, bool revOrder) const {
    std::string ret;
    for(size_t i = 0; i < depth; ++i) {
      ret += " ";
    }
    ret += mSourceNode.dump(tok, revOrder);
    ret += "\n";
    return ret;
  }
  std::vector<sp<ErrorTree>> mChildren;
  wp<ErrorTree> mParent;
  const ExpressionFailInfo& mSourceNode;
};
sp<ErrorTree> createErrorTree(const ExpressionFailInfo& info, const PegTokenizer& tokenizer, wp<ErrorTree> parent) {
  auto tree = std::make_shared<ErrorTree>(parent, info);
  for(const auto& child: info.mSubExprFailInfo) {
    tree->attach(createErrorTree(child, tokenizer, tree));
  }
  return tree;
}
std::string errorsToString(const ParsingFailInfo& info, const PegTokenizer& tokenizer) {
  auto tree = createErrorTree(*info.error, tokenizer, wp<ErrorTree>{});
  // walk to the last node in the tree
  sp<ErrorTree> lastNode = tree;
  while(lastNode) {
    if(lastNode->getChildren().empty()) {
      break;
    } else {
      lastNode = lastNode->getChildren().at(lastNode->getChildren().size() - 1);
    }
  }
  /*size_t stepsToSuccess = 0;
  assert(lastNode);
  // walk up from the last node until we encounter something like Success or an additional error
  sp<ErrorTree> lastSuccessNode = lastNode;
  while(lastSuccessNode) {
    if(lastSuccessNode->getSourceNode().isStoppingPoint()) {
      break;
    }
    auto next = lastSuccessNode->getParent().lock();
    if(!next) {
      break;
    }
    lastSuccessNode = next;
    stepsToSuccess += 1;
  }
  assert(lastSuccessNode);*/
  std::string ret;
  if(info.eof) {
    ret += "Unexpected EOF\n";
  }
  ret += tree->dump(tokenizer, 0);// + "\n" + lastNode->dumpReverse(tokenizer, 0, stepsToSuccess);
  return ret;
}
}