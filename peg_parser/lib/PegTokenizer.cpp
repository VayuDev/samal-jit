#include <cassert>
#include "peg_parser/PegTokenizer.hpp"

namespace peg {

PegTokenizer::PegTokenizer(std::string code)
    : mCode(std::move(code)) {
  mStates.push(0);
}
const char *PegTokenizer::pushState() {
  assert(!mStates.empty());
  mStates.push(mStates.top());
  return &mCode.at(mStates.top());
}
const char *PegTokenizer::popState() {
  assert(!mStates.empty());
  auto state = mStates.top();
  mStates.pop();
  assert(!mStates.empty());
  return &mCode.at(state);
}
const char *PegTokenizer::updateState() {
  auto state = mStates.top();
  mStates.pop();
  mStates.top() = state;
  return &mCode.at(state);
}
bool PegTokenizer::matchString(const std::string_view &string) {
  assert(!mStates.empty());
  skipWhitespaces();
  for(size_t i = 0; i < string.size(); ++i) {
    if(i >= mCode.size()) {
      return false;
    }
    if(string.at(i) != mCode.at(mStates.top() + i)) {
      return false;
    }
  }
  mStates.top() += string.size();
  skipWhitespaces();
  return true;
}
bool PegTokenizer::matchRegex(const std::regex &regex) {
  skipWhitespaces();
  auto it = std::sregex_iterator (mCode.cbegin() + mStates.top(), mCode.cend(), regex);
  if(it == std::sregex_iterator()) {
    return false;
  }
  mStates.top() += it->size();
  skipWhitespaces();
  return true;
}
void PegTokenizer::skipWhitespaces() {
  const static std::string WHITESPACE_CHARS{"\t \n"};
  assert(!mStates.empty());
  while(mStates.top() < mCode.size() && WHITESPACE_CHARS.find(mCode.at(mStates.top())) != std::string::npos) {
    mStates.top() += 1;
  }
}
bool PegTokenizer::isEmpty() const {
  assert(!mStates.empty());
  assert(mStates.top() <= mCode.size());
  return mStates.top() == mCode.size();
}

}