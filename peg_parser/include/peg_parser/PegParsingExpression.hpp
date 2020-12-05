#pragma once

#include <string>
#include <optional>
#include <vector>
#include <regex>
#include <variant>
#include <any>
#include "peg_parser/PegUtil.hpp"
#include "peg_parser/PegForward.hpp"
#include "peg_parser/ParsingState.hpp"

namespace peg {


enum class RuleFailReason {
  MISSING_SEQUENCE
};

class ExpressionFailInfo {
 public:

 private:
  RuleFailReason mReason;
};

using RuleMap = std::map<std::string, sp<ParsingExpression>>;

struct MatchInfo {
  const char* start = nullptr;
  const char* end = nullptr;
  std::optional<size_t> choice;
  std::any result;
  std::vector<MatchInfo> subs;
};

using RuleResult = std::variant<std::pair<ParsingState, MatchInfo>, ExpressionFailInfo>;

class ParsingExpression {
 public:
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

}