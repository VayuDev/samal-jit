#pragma once
#include <string>
#include <stack>
#include <string_view>
#include <regex>
#include "peg_parser/PegStructs.hpp"

namespace peg {

class PegTokenizer {
 public:
  explicit PegTokenizer(std::string code);
  [[nodiscard]] const char* getPtr(ParsingState state) const;
  [[nodiscard]] ParsingState skipWhitespaces(ParsingState) const;
  [[nodiscard]] std::optional<ParsingState> matchString(ParsingState, const std::string_view& string) const;
  [[nodiscard]] std::optional<ParsingState> matchRegex(ParsingState, const std::regex& regex) const;
  [[nodiscard]] bool isEmpty(ParsingState) const;
  [[nodiscard]] std::pair<size_t, size_t> getPosition(ParsingState state) const;
 private:
  std::string mCode;
};

}
