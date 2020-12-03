#pragma once
#include <string>
#include <stack>
#include <string_view>
#include <regex>

namespace peg {

class PegTokenizer {
 public:
  explicit PegTokenizer(std::string code);
  const char* pushState();
  const char* popState();
  const char* updateState();
  bool matchString(const std::string_view& string);
  bool matchRegex(const std::regex& regex);
  bool isEmpty() const;
 private:
  void skipWhitespaces();
  std::string mCode;
  std::stack<size_t> mStates;
};

}
