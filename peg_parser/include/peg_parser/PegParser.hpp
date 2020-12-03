#pragma once

#include <string>
#include <any>
#include <string_view>
#include <map>
#include <variant>
#include <memory>
#include <vector>
#include <regex>
#include <tuple>

#include "peg_parser/PegTokenizer.hpp"

namespace peg {

template<typename T>
using sp = std::shared_ptr<T>;

template<typename T>
using up = std::unique_ptr<T>;

enum class RuleFailReason {
  MISSING_SEQUENCE
};

class ExpressionFailInfo {
 public:

 private:
  RuleFailReason mReason;
};

struct ParsingState {
  PegTokenizer& tokenizer;
};

using RuleResult = std::variant<std::pair<ParsingState, std::any>, ExpressionFailInfo>;


class ParsingExpression {
 public:
  [[nodiscard]] virtual RuleResult match(ParsingState) const = 0;
  virtual std::string dump() const = 0;
 private:
};

class TerminalParsingExpression : public ParsingExpression {
 public:
  explicit TerminalParsingExpression(std::string value);
  explicit TerminalParsingExpression(std::string stringRep, std::regex value);
  [[nodiscard]] RuleResult match(ParsingState) const override;
  [[nodiscard]] std::string dump() const override;
 private:
  std::string mStringRepresentation;
  std::optional<std::regex> mRegex;
};

class SequenceParsingExpression : public ParsingExpression {
 public:
  explicit SequenceParsingExpression(std::vector<sp<ParsingExpression>> children);
  [[nodiscard]] RuleResult match(ParsingState) const override;
  [[nodiscard]] std::string dump() const override;
 private:
  std::vector<sp<ParsingExpression>> mChildren;
};

class ChoiceParsingExpression : public ParsingExpression {
 public:
  explicit ChoiceParsingExpression(std::vector<sp<ParsingExpression>> children);
  [[nodiscard]] RuleResult match(ParsingState) const override;
  [[nodiscard]] std::string dump() const override;
 private:
  std::vector<sp<ParsingExpression>> mChildren;
};

class PegParser {
 public:
  void addRule(std::string nonTerminal, std::unique_ptr<ParsingExpression> rule);
  std::any parse(const std::string_view& start, std::string code);
 private:
  std::map<std::string, up<ParsingExpression>> mRules;
};

}