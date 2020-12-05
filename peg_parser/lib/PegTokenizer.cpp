#include <cassert>
#include "peg_parser/PegTokenizer.hpp"

namespace peg {

PegTokenizer::PegTokenizer(std::string code)
    : mCode(std::move(code)) {
}

std::optional<ParsingState> PegTokenizer::matchString(ParsingState state, const std::string_view &string) const {
  state = skipWhitespaces(state);
  for(size_t i = 0; i < string.size(); ++i) {
    if(i >= mCode.size()) {
      return {};
    }
    if(string.at(i) != mCode.at(state.tokenizerState + i)) {
      return {};
    }
  }
  state.tokenizerState += string.size();
  state = skipWhitespaces(state);
  return state;
}
std::optional<ParsingState> PegTokenizer::matchRegex(ParsingState state, const std::regex &regex) const {
  state = skipWhitespaces(state);
  auto it = std::sregex_iterator (mCode.cbegin() + state.tokenizerState, mCode.cend(), regex);
  if(it == std::sregex_iterator()) {
    return {};
  }
  state.tokenizerState += it->size();
  state = skipWhitespaces(state);
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
}