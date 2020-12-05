#pragma once
#include <cstddef>
#include <functional>
#include <vector>
#include <optional>
#include <any>
#include "peg_parser/PegUtil.hpp"
#include "peg_parser/PegForward.hpp"

namespace peg {

struct ParsingState {
  size_t tokenizerState = 0;
};

struct MatchInfo {
  const char* start = nullptr;
  const char* end = nullptr;
  std::optional<size_t> choice;
  std::any result;
  std::vector<MatchInfo> subs;
};

using RuleCallback = std::function<std::any(const MatchInfo& info)>;

struct Rule {
  sp<ParsingExpression> expr;
  RuleCallback callback;
};

using RuleMap = std::map<std::string, Rule>;

}