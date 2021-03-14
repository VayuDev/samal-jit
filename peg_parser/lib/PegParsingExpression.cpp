#include "peg_parser/PegParsingExpression.hpp"
#include "peg_parser/PegTokenizer.hpp"
#include <cassert>

namespace peg {

const std::regex gNextStringRegex{ "^[^ \n\t]*", std::regex::optimize };

std::string ExpressionFailInfo::dump(const PegTokenizer& tokenizer, bool reverseOrder) const {
    auto [line, column] = tokenizer.getPosition(mState);
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
        switch(mReason) {
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
        case ExpressionFailReason::UNMATCHED_REGEX:
            msg += "Expected regex '" + mInfo1 + "', got: '" + mInfo2 + "'";
            break;
        case ExpressionFailReason::UNMATCHED_STRING:
            msg += "Expected '" + mInfo1 + "', got: '" + mInfo2 + "'";
            break;
        case ExpressionFailReason::ADDITIONAL_ERROR_MESSAGE:
            msg += mInfo1 + ", got: '" + mInfo2 + "'\033[0m";
            break;
        case ExpressionFailReason::CALLBACK_THREW_EXCEPTION:
            msg += "Exception in callback: " + mInfo1;
            break;
        default:
            assert(false);
        }
    } else {
        switch(mReason) {
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
        case ExpressionFailReason::UNMATCHED_REGEX:
            msg += "Expected regex '" + mInfo1 + "', got: '" + mInfo2 + "'";
            break;
        case ExpressionFailReason::UNMATCHED_STRING:
            msg += "Expected '" + mInfo1 + "', got: '" + mInfo2 + "'";
            break;
        case ExpressionFailReason::ADDITIONAL_ERROR_MESSAGE:
            msg += mInfo1 + " (" + mInfo2 + ")\033[0m";
            break;
        case ExpressionFailReason::CALLBACK_THREW_EXCEPTION:
            msg += "Exception in callback: " + mInfo1;
            break;
        default:
            assert(false);
        }
    }

    return msg;
}

SequenceParsingExpression::SequenceParsingExpression(std::vector<sp<ParsingExpression>> children)
: mChildren(std::move(children)) {
}
RuleResult SequenceParsingExpression::match(ParsingState state, const RuleMap& rules, const PegTokenizer& tokenizer) const {
    auto startState = state;
    auto startPtr = tokenizer.getPtr(state);
    std::vector<MatchInfo> childrenResults;
    std::vector<ExpressionFailInfo> childrenFailReasons;
    for(auto& child : mChildren) {
        auto childRet = child->match(state, rules, tokenizer);
        if(childRet.index() == 0) {
            // child matched
            state = std::get<0>(childRet).getState();
            childrenResults.emplace_back(std::get<0>(childRet).moveMatchInfo());
            childrenFailReasons.emplace_back(std::get<0>(childRet).moveFailInfo());
        } else {
            // child failed
            childrenFailReasons.emplace_back(std::move(std::get<1>(childRet)));
            return ExpressionFailInfo{
                state,
                dump(),
                ExpressionFailReason::SEQUENCE_CHILD_FAILED,
                std::string{ std::string_view(startPtr, tokenizer.getPtr(state) - startPtr) },
                "",
                std::move(childrenFailReasons)
            };
        }
    }
    return ExpressionSuccessInfo{
        state,
        MatchInfo{
            .start = startPtr,
            .len = static_cast<size_t>(tokenizer.getPtr(state) - startPtr),
            .sourcePosition = tokenizer.getPosition(startState),
            .subs = std::move(childrenResults) },
        ExpressionFailInfo{ state, dump(), std::move(childrenFailReasons) }
    };
}
std::string SequenceParsingExpression::dump() const {
    std::string ret;
    for(auto it = mChildren.cbegin(); it != mChildren.cend(); it++) {
        ret += (*it)->dump();
        if(std::next(it) != mChildren.cend()) {
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
    auto startState = state;
    auto startPtr = tokenizer.getPtr(state);
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
                .start = startPtr,
                .len = static_cast<size_t>(tokenizer.getPtr(*tokenizerRet) - startPtr),
                .sourcePosition = tokenizer.getPosition(startState) },
            ExpressionFailInfo{ state, dump() }
        };
    } else {
        state = tokenizer.skipWhitespaces(state);
        size_t len = tokenizer.getPtr(*tokenizer.matchRegex(state, gNextStringRegex)) - tokenizer.getPtr(state);
        std::string failedString{ std::string_view{ tokenizer.getPtr(state), len } };
        return ExpressionFailInfo{
            state,
            dump(),
            mRegex ? ExpressionFailReason::UNMATCHED_REGEX : ExpressionFailReason::UNMATCHED_STRING,
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
    for(auto& child : mChildren) {
        auto childRet = child->match(state, rules, tokenizer);
        if(childRet.index() == 0) {
            // child matched
            std::vector<MatchInfo> subs;
            auto start = std::get<0>(childRet).getMatchInfo().start;
            auto len = std::get<0>(childRet).getMatchInfo().len;
            auto pos = std::get<0>(childRet).getMatchInfo().sourcePosition;
            subs.emplace_back(std::get<0>(childRet).moveMatchInfo());
            childrenFailReasons.push_back(std::get<0>(childRet).moveFailInfo());
            return ExpressionSuccessInfo{
                std::get<0>(childRet).getState(),
                MatchInfo{
                    .start = start,
                    .len = len,
                    .sourcePosition = pos,
                    .choice = i,
                    .subs = std::move(subs) },
                ExpressionFailInfo{ state, dump(), std::move(childrenFailReasons) }
            };
        } else {
            childrenFailReasons.emplace_back(std::get<1>(childRet));
        }
        i++;
    }
    return ExpressionFailInfo{
        state,
        dump(),
        ExpressionFailReason::CHOICE_NO_CHILD_SUCCEEDED,
        std::move(childrenFailReasons)
    };
}
std::string ChoiceParsingExpression::dump() const {
    std::string ret = "(";
    for(auto it = mChildren.cbegin(); it != mChildren.cend(); it++) {
        ret += (*it)->dump();
        if(std::next(it) != mChildren.cend()) {
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
    auto startState = state;
    auto startPtr = tokenizer.getPtr(state);
    auto maybeRule = rules.find(mNonTerminal);
    if(maybeRule == rules.end()) {
        throw std::runtime_error{"No rule found for nonterminal '" + mNonTerminal + "'"};
    }
    auto& rule = maybeRule->second;
    auto ruleRetValue = rule.expr->match(state, rules, tokenizer);
    if(ruleRetValue.index() == 1) {
        // error in child
        return ruleRetValue;
    }
    Any callbackResult;
    if(rule.callback) {
        try {
            callbackResult = rule.callback(std::get<0>(ruleRetValue).getMatchInfoMut());
        } catch(std::exception& e) {
            return ExpressionFailInfo{
                std::get<0>(ruleRetValue).getState(),
                dump(),
                ExpressionFailReason::CALLBACK_THREW_EXCEPTION,
                e.what()
            };
        }
    }
    auto len = static_cast<size_t>(tokenizer.getPtr(std::get<0>(ruleRetValue).getState()) - startPtr);

    std::vector<MatchInfo> subs;
    subs.emplace_back(std::get<0>(ruleRetValue).moveMatchInfo());
    return ExpressionSuccessInfo{ std::get<0>(ruleRetValue).getState(), MatchInfo{ .start = startPtr, .len = len, .sourcePosition = tokenizer.getPosition(startState), .result = std::move(callbackResult), .subs = std::move(subs) }, std::get<0>(ruleRetValue).moveFailInfo() };
}
std::string NonTerminalParsingExpression::dump() const {
    return mNonTerminal;
}
OptionalParsingExpression::OptionalParsingExpression(sp<ParsingExpression> child)
: mChild(std::move(child)) {
}
RuleResult OptionalParsingExpression::match(ParsingState state, const RuleMap& rules, const PegTokenizer& tokenizer) const {
    auto startState = state;
    auto childRet = mChild->match(state, rules, tokenizer);
    if(childRet.index() == 1) {
        // error
        return ExpressionSuccessInfo{ state, MatchInfo{
                                                 .start = tokenizer.getPtr(startState),
                                                 .len = 0,
                                                 .sourcePosition = tokenizer.getPosition(state),
                                             },
            ExpressionFailInfo{ state, dump(), { std::move(std::get<1>(childRet)) } } };
    }
    // success
    state = std::get<0>(childRet).getState();
    std::vector<MatchInfo> subs;
    subs.emplace_back(std::get<0>(childRet).moveMatchInfo());
    return ExpressionSuccessInfo{ state, MatchInfo{ .start = tokenizer.getPtr(startState), .len = static_cast<size_t>(tokenizer.getPtr(state) - tokenizer.getPtr(startState)), .sourcePosition = tokenizer.getPosition(startState), .subs = std::move(subs) }, ExpressionFailInfo{ state, dump(), { std::get<0>(childRet).moveFailInfo() } } };

    assert(false);
}
std::string OptionalParsingExpression::dump() const {
    return "(" + mChild->dump() + ")?";
}
OneOrMoreParsingExpression::OneOrMoreParsingExpression(sp<ParsingExpression> child)
: mChild(std::move(child)) {
}
RuleResult OneOrMoreParsingExpression::match(ParsingState state, const RuleMap& rules, const PegTokenizer& tokenizer) const {
    auto startState = state;
    auto startPtr = tokenizer.getPtr(state);
    auto childRet = mChild->match(state, rules, tokenizer);
    if(childRet.index() == 1) {
        // error
        return ExpressionFailInfo{ state, dump(), ExpressionFailReason::REQUIRED_ONE_OR_MORE, { std::get<1>(childRet) } };
    }
    // success
    state = std::get<0>(childRet).getState();
    std::vector<MatchInfo> childrenResults;
    std::vector<ExpressionFailInfo> childrenFailReasons;
    childrenResults.emplace_back(std::get<0>(childRet).moveMatchInfo());
    childrenFailReasons.emplace_back(std::get<0>(childRet).moveFailInfo());
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
    return ExpressionSuccessInfo{ state, MatchInfo{ .start = startPtr, .len = static_cast<size_t>(tokenizer.getPtr(state) - startPtr), .sourcePosition = tokenizer.getPosition(startState), .subs = std::move(childrenResults) }, ExpressionFailInfo{ state, dump(), std::move(childrenFailReasons) } };
}
std::string OneOrMoreParsingExpression::dump() const {
    return "(" + mChild->dump() + ")+";
}
ZeroOrMoreParsingExpression::ZeroOrMoreParsingExpression(sp<ParsingExpression> child)
: mChild(std::move(child)) {
}
RuleResult ZeroOrMoreParsingExpression::match(ParsingState state, const RuleMap& rules, const PegTokenizer& tokenizer) const {
    auto startState = state;
    auto startPtr = tokenizer.getPtr(state);
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
    return ExpressionSuccessInfo{ state, MatchInfo{ .start = startPtr, .len = static_cast<size_t>(tokenizer.getPtr(state) - startPtr), .sourcePosition = tokenizer.getPosition(startState), .subs = std::move(childrenResults) }, ExpressionFailInfo{ state, dump(), std::move(childrenFailReasons) } };
}
std::string ZeroOrMoreParsingExpression::dump() const {
    return "(" + mChild->dump() + ")*";
}
ErrorMessageInfoExpression::ErrorMessageInfoExpression(sp<ParsingExpression> child, std::string error)
: mChild(std::move(child)), mErrorMsg(std::move(error)) {
}
RuleResult ErrorMessageInfoExpression::match(ParsingState state, const RuleMap& rules, const PegTokenizer& tokenizer) const {
    auto childRes = mChild->match(state, rules, tokenizer);
    if(childRes.index() == 0) {
        return childRes;
    }
    state = tokenizer.skipWhitespaces(state);
    size_t len = tokenizer.getPtr(*tokenizer.matchRegex(state, gNextStringRegex)) - tokenizer.getPtr(state);
    std::string failedString{ std::string_view{ tokenizer.getPtr(state), len } };
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
SkipWhitespacesExpression::SkipWhitespacesExpression(sp<ParsingExpression> child)
: mChild(std::move(child)) {
}
RuleResult SkipWhitespacesExpression::match(ParsingState state, const RuleMap& rules, const PegTokenizer& tokenizer) const {
    state = tokenizer.skipWhitespaces(state);
    return mChild->match(state, rules, tokenizer);
}
std::string SkipWhitespacesExpression::dump() const {
    return mChild->dump();
}
DoNotSkipWhitespacesExpression::DoNotSkipWhitespacesExpression(sp<ParsingExpression> child)
: mChild(std::move(child)) {
}
RuleResult DoNotSkipWhitespacesExpression::match(ParsingState state, const RuleMap& rules, const PegTokenizer& tokenizer) const {
    return mChild->match(state, rules, tokenizer);
}
std::string DoNotSkipWhitespacesExpression::dump() const {
    return "~nws~(" + mChild->dump() + ")";
}
ForceSkippingWhitespacesExpression::ForceSkippingWhitespacesExpression(sp<ParsingExpression> child)
: mChild(std::move(child)) {
}
RuleResult ForceSkippingWhitespacesExpression::match(ParsingState state,
    const RuleMap& rules,
    const PegTokenizer& tokenizer) const {
    auto newState = tokenizer.skipWhitespaces(state);
    if(newState == state) {
        return ExpressionFailInfo{ state, dump(), ExpressionFailReason::REQUIRED_WHITESPACES, std::vector<ExpressionFailInfo>{} };
    }
    return mChild->match(newState, rules, tokenizer);
}
std::string ForceSkippingWhitespacesExpression::dump() const {
    return "~fws~(" + mChild->dump() + ")";
}

SkipWhitespacesNoNewlinesExpression::SkipWhitespacesNoNewlinesExpression(sp<ParsingExpression> child)
: mChild(std::move(child)) {
}
RuleResult SkipWhitespacesNoNewlinesExpression::match(ParsingState state,
    const RuleMap& rules,
    const PegTokenizer& tokenizer) const {
    state = tokenizer.skipWhitespaces(state, false);
    return mChild->match(state, rules, tokenizer);
}
std::string SkipWhitespacesNoNewlinesExpression::dump() const {
    return "~snn~(" + mChild->dump() + ")";
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
    [[nodiscard]] auto& getChildren() {
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
        for(const auto& child : mChildren) {
            ret += child->dump(tok, depth + 1);
        }
        return ret;
    }
    [[nodiscard]] std::string dumpReverse(const PegTokenizer& tok, size_t depth, size_t maxDepth) const {
        if(depth >= maxDepth) {
            return "";
        }
        std::string ret;
        for(const auto& child : mChildren) {
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
    for(const auto& child : info.mSubExprFailInfo) {
        tree->attach(createErrorTree(child, tokenizer, tree));
    }
    return tree;
}
std::string errorsToString(const ParsingFailInfo& info, const PegTokenizer& tokenizer) {
    auto tree = createErrorTree(*info.error, tokenizer, wp<ErrorTree>{});

    /*
  // walk to the last node in the tree
  sp<ErrorTree> lastNode = tree;
  while(lastNode) {
    if(lastNode->getChildren().empty()) {
      break;
    } else {
      lastNode = lastNode->getChildren().at(lastNode->getChildren().size() - 1);
    }
  }
  size_t stepsToSuccess = 0;
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
    size_t lastLine = 0;
    sp<ErrorTree> node = tree;
    std::function<void(sp<ErrorTree>)> search = [&](sp<ErrorTree> node) {
        auto [line, column] = tokenizer.getPosition(node->getSourceNode().getState());
        if(line > lastLine) {
            lastLine = line;
        }
        for(auto& child : node->getChildren()) {
            search(child);
        }
    };
    search(tree);

    std::function<bool(sp<ErrorTree>)> prune = [&](sp<ErrorTree> node) -> bool {
        auto [line, column] = tokenizer.getPosition(node->getSourceNode().getState());
        for(ssize_t i = node->getChildren().size() - 1; i >= 0; --i) {
            if(prune(node->getChildren().at(i))) {
                node->getChildren().erase(node->getChildren().cbegin() + i);
            }
        }
        return line != lastLine && node->getChildren().empty();
    };
    prune(tree);

    std::string ret;
    if(info.eof) {
        ret += "Unexpected EOF\n";
    }
    ret += tree->dump(tokenizer, 0); // + "\n" + lastNode->dumpReverse(tokenizer, 0, stepsToSuccess);
    return ret;
}
}