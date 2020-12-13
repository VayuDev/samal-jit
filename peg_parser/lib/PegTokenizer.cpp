#include <cassert>
#include "peg_parser/PegTokenizer.hpp"

namespace peg {

PegTokenizer::PegTokenizer(std::string code)
    : mCode(std::move(code)) {
}

std::optional<ParsingState> PegTokenizer::matchString(ParsingState state, const std::string_view &string) const {
  bool skippedWhiteSpaces = false;
  for(ssize_t i = 0; i < string.size(); ++i) {
    if(state.tokenizerState + i >= mCode.size()) {
      return {};
    }
    if(string.at(i) != mCode.at(state.tokenizerState + i)) {
      if(i == 0 && !skippedWhiteSpaces) {
        state = skipWhitespaces(state);
        i = -1;
        skippedWhiteSpaces = true;
      } else {
        return {};
      }
    }
  }
  state.tokenizerState += string.size();
  return state;
}
std::optional<ParsingState> PegTokenizer::matchRegex(ParsingState state, const std::regex &regex) const {
  auto it = std::sregex_iterator (mCode.cbegin() + state.tokenizerState, mCode.cend(), regex);
  if(it == std::sregex_iterator()) {
    state = skipWhitespaces(state);
    it = std::sregex_iterator (mCode.cbegin() + state.tokenizerState, mCode.cend(), regex);
    if(it == std::sregex_iterator()) {
      return {};
    }
  }
  state.tokenizerState += it->length();
  return state;
}

ParsingState PegTokenizer::skipWhitespaces(ParsingState state)  const {
  const static std::string WHITESPACE_CHARS{"\t \n"};
  while(state.tokenizerState < mCode.size() && WHITESPACE_CHARS.find(mCode.at(state.tokenizerState)) != std::string::npos) {
    state.tokenizerState += 1;
  }
  return state;
}
bool PegTokenizer::isEmpty(ParsingState state) const {
  return state.tokenizerState >= mCode.size();
}
const char *PegTokenizer::getPtr(ParsingState state) const {
  if(state.tokenizerState >= mCode.size()) {
    return &*mCode.cend();
  }
  return &mCode.at(state.tokenizerState);
}
std::pair<size_t, size_t> PegTokenizer::getPosition(ParsingState state) const {
  size_t line = 1, column = 1;
  for(char c: mCode) {
    if(c == '\n') {
      line += 1;
      column = 1;
    } else {
      column += 1;
    }
  }
  return std::make_pair(line, column);
}
}