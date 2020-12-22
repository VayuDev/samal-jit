#pragma once

#include <string>
#include <optional>
#include <vector>
#include <regex>
#include <variant>
#include <any>
#include <cassert>
#include "peg_parser/PegUtil.hpp"
#include "peg_parser/PegForward.hpp"
#include "peg_parser/PegStructs.hpp"

namespace peg {

enum class ExpressionFailReason {
  // It doesn't make much sense to have a success-state.
  // The reason why we have it is that if e.g. a ZeroOrMoreExpression
  // wants to tell us why it didn't match any more stuff, we need
  // to propagate it's ExpressionFailInfo back up the call stack
  // so that we have the ability to display it when there are tokens
  // left unconsumed. Because of that, every ParsingExpression needs
  // to always return an ExpressionFailReason, even if it succeeds.
  SUCCESS,
  SEQUENCE_CHILD_FAILED,
  CHOICE_NO_CHILD_SUCCEEDED,
  ADDITIONAL_ERROR_MESSAGE,
  REQUIRED_ONE_OR_MORE,
  REQUIRED_WHITESPACES,
  UNMATCHED_REGEX,
  UNMATCHED_STRING,
};

class ExpressionFailInfo {
 public:
  explicit ExpressionFailInfo(ParsingState state, std::string selfDump)
  : mState(state), mSelfDump(std::move(selfDump)) {

  }
  explicit ExpressionFailInfo(ParsingState state, std::string selfDump, ExpressionFailReason reason, std::string info1, std::string info2)
  : mReason(reason), mState(state), mSelfDump(std::move(selfDump)), mInfo1(std::move(info1)), mInfo2(std::move(info2)){

  }
  explicit ExpressionFailInfo(ParsingState state, std::string selfDump, ExpressionFailReason reason, std::vector<ExpressionFailInfo> subExprFailInfo)
      : mReason(reason), mState(state), mSelfDump(std::move(selfDump)), mSubExprFailInfo(std::move(subExprFailInfo)){

  }
  explicit ExpressionFailInfo(ParsingState state, std::string selfDump, ExpressionFailReason reason, std::string info, std::string info2, std::vector<ExpressionFailInfo> subExprFailInfo)
      : mReason(reason), mState(state), mSelfDump(std::move(selfDump)), mInfo1(std::move(info)), mInfo2(std::move(info2)), mSubExprFailInfo(std::move(subExprFailInfo)){

  }
  // Assumes success
  explicit ExpressionFailInfo(ParsingState state, std::string selfDump, std::vector<ExpressionFailInfo> subExprFailInfo)
      : mReason(ExpressionFailReason::SUCCESS), mState(state), mSelfDump(std::move(selfDump)), mSubExprFailInfo(std::move(subExprFailInfo)){

  }
  [[nodiscard]] std::string dump(const PegTokenizer& tokenizer, bool reverseOrder = false) const;
  [[nodiscard]] bool isSuccess() const {
    return mReason == ExpressionFailReason::SUCCESS;
  }
  [[nodiscard]] bool isFromSequenceExpression() const {
    return mReason == ExpressionFailReason::SEQUENCE_CHILD_FAILED;
  }
  [[nodiscard]] bool isAdditionalErrorMessage() const {
    return mReason == ExpressionFailReason::ADDITIONAL_ERROR_MESSAGE;
  }
  [[nodiscard]] auto getState() const {
    return mState;
  }
 private:
  ExpressionFailReason mReason { ExpressionFailReason::SUCCESS };
  ParsingState mState;
  std::string mSelfDump, mInfo1, mInfo2;
  std::vector<ExpressionFailInfo> mSubExprFailInfo;
  friend sp<class ErrorTree> createErrorTree(const ExpressionFailInfo& info, const PegTokenizer& tokenizer, wp<ErrorTree> parent);
};

std::string errorsToString(const ParsingFailInfo&, const PegTokenizer&);

class ExpressionSuccessInfo {
 public:
  ExpressionSuccessInfo(ParsingState state, MatchInfo&& matchInfo, ExpressionFailInfo&& expressionFailInfo)
  : mState(state), mMatchInfo(std::move(matchInfo)), mFailInfo(std::move(expressionFailInfo)) {

  }
  [[nodiscard]] ParsingState getState() const {
    return mState;
  }
  [[nodiscard]] MatchInfo&& moveMatchInfo() & {
    return std::move(mMatchInfo);
  }
  [[nodiscard]] const MatchInfo& getMatchInfo() const & {
    return mMatchInfo;
  }
  [[nodiscard]] MatchInfo& getMatchInfoMut() {
    return mMatchInfo;
  }
  [[nodiscard]] ExpressionFailInfo moveFailInfo() {
    return std::move(mFailInfo);
  }
 private:
  ParsingState mState;
  MatchInfo mMatchInfo;
  ExpressionFailInfo mFailInfo;
};

using RuleResult = std::variant<ExpressionSuccessInfo, ExpressionFailInfo>;

class ParsingExpression {
 public:
  virtual ~ParsingExpression() = default;
  [[nodiscard]] virtual RuleResult match(ParsingState state, const RuleMap& rules, const PegTokenizer& tokenizer) const = 0;
  [[nodiscard]] virtual std::string dump() const = 0;
 private:
};

class TerminalParsingExpression : public ParsingExpression {
 public:
  explicit TerminalParsingExpression(std::string value);
  explicit TerminalParsingExpression(std::string stringRep, std::regex value);
  [[nodiscard]] RuleResult match(ParsingState, const RuleMap&, const PegTokenizer& tokenizer) const override;
  [[nodiscard]] std::string dump() const override;
 private:
  std::string mStringRepresentation;
  std::optional<std::regex> mRegex;
};

class NonTerminalParsingExpression : public ParsingExpression {
 public:
  explicit NonTerminalParsingExpression(std::string value);
  [[nodiscard]] RuleResult match(ParsingState, const RuleMap&, const PegTokenizer& tokenizer) const override;
  [[nodiscard]] std::string dump() const override;
 private:
  std::string mNonTerminal;
};

class OptionalParsingExpression : public ParsingExpression {
 public:
  explicit OptionalParsingExpression(sp<ParsingExpression> child);
  [[nodiscard]] RuleResult match(ParsingState, const RuleMap&, const PegTokenizer& tokenizer) const override;
  [[nodiscard]] std::string dump() const override;
 private:
  sp<ParsingExpression> mChild;
};

class OneOrMoreParsingExpression : public ParsingExpression {
 public:
  explicit OneOrMoreParsingExpression(sp<ParsingExpression> child);
  [[nodiscard]] RuleResult match(ParsingState, const RuleMap&, const PegTokenizer& tokenizer) const override;
  [[nodiscard]] std::string dump() const override;
 private:
  sp<ParsingExpression> mChild;
};

class ZeroOrMoreParsingExpression : public ParsingExpression {
 public:
  explicit ZeroOrMoreParsingExpression(sp<ParsingExpression> child);
  [[nodiscard]] RuleResult match(ParsingState, const RuleMap&, const PegTokenizer& tokenizer) const override;
  [[nodiscard]] std::string dump() const override;
 private:
  sp<ParsingExpression> mChild;
};

class SequenceParsingExpression : public ParsingExpression {
 public:
  explicit SequenceParsingExpression(std::vector<sp<ParsingExpression>> children);
  [[nodiscard]] RuleResult match(ParsingState, const RuleMap&, const PegTokenizer& tokenizer) const override;
  [[nodiscard]] std::string dump() const override;
 private:
  std::vector<sp<ParsingExpression>> mChildren;
};

class ChoiceParsingExpression : public ParsingExpression {
 public:
  explicit ChoiceParsingExpression(std::vector<sp<ParsingExpression>> children);
  [[nodiscard]] RuleResult match(ParsingState, const RuleMap&, const PegTokenizer& tokenizer) const override;
  [[nodiscard]] std::string dump() const override;
 private:
  std::vector<sp<ParsingExpression>> mChildren;
};

class ErrorMessageInfoExpression : public ParsingExpression {
 public:
  explicit ErrorMessageInfoExpression(sp<ParsingExpression> child, std::string error);
  [[nodiscard]] RuleResult match(ParsingState, const RuleMap&, const PegTokenizer& tokenizer) const override;
  [[nodiscard]] std::string dump() const override;
 private:
  sp<ParsingExpression> mChild;
  std::string mErrorMsg;
};

class SkipWhitespacesExpression : public ParsingExpression {
 public:
  explicit SkipWhitespacesExpression(sp<ParsingExpression> child);
  [[nodiscard]] RuleResult match(ParsingState, const RuleMap&, const PegTokenizer& tokenizer) const override;
  [[nodiscard]] std::string dump() const override;
 private:
  sp<ParsingExpression> mChild;
};

class DoNotSkipWhitespacesExpression : public ParsingExpression {
 public:
  explicit DoNotSkipWhitespacesExpression(sp<ParsingExpression> child);
  [[nodiscard]] RuleResult match(ParsingState, const RuleMap&, const PegTokenizer& tokenizer) const override;
  [[nodiscard]] std::string dump() const override;
 private:
  sp<ParsingExpression> mChild;
};

class ForceSkippingWhitespacesExpression : public ParsingExpression {
 public:
  explicit ForceSkippingWhitespacesExpression(sp<ParsingExpression> child);
  [[nodiscard]] RuleResult match(ParsingState, const RuleMap&, const PegTokenizer& tokenizer) const override;
  [[nodiscard]] std::string dump() const override;
 private:
  sp<ParsingExpression> mChild;
};

}