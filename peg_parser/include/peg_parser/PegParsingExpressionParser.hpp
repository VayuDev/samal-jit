#pragma once
#include "peg_parser/PegParser.hpp"
#include <cassert>

namespace peg {

sp<ParsingExpression> stringToParsingExpression(const std::string_view &);

// Just for the tests here
class ExpressionTokenizer {
 public:
  explicit inline ExpressionTokenizer(const std::string_view& expr)
      : mExprString(expr) {
    genNextToken();
  }
  [[nodiscard]] inline const std::optional<std::string_view>& currentToken() const {
    return mCurrentToken;
  }
  inline void advance() {
    genNextToken();
  }
 private:

  inline void genNextToken() {
    const static std::string STOP_CHARS{".)({}+-|/"};
    skipWhitespaces();
    auto start = mOffset;
    if(mOffset < mExprString.size()) {
      switch(getCurrentChar()) {
        case '\'':
          consumeString();
          break;
        case '[':
          consumeRegex();
          break;
        case '+':
        case '*':
        case ')':
        case '(':
        case '|':
        case '/':
          mOffset += 1;
          break;
        default:
          if(isalnum(getCurrentChar())) {
            consumeNonTerminal();
          } else {
            throw std::runtime_error{std::string{"Invalid input char: '"} + getCurrentChar() + "\'"};
          }
      }
      mCurrentToken = mExprString.substr(start, mOffset - start);
    } else {
      mCurrentToken = {};
    }
  }
  inline void consumeNonTerminal() {
    while(mOffset < mExprString.size() && isalnum(getCurrentChar())) {
      mOffset += 1;
    }
  }
  inline void consumeString() {
    assert(getCurrentChar() == '\'');
    mOffset += 1;
    bool escapingChar = false;
    while(true) {
      if(mOffset >= mExprString.size()) {
        throw std::runtime_error{"Unterminated string in expression!"};
      }
      if(getCurrentChar() == '\\' && !escapingChar) {
        escapingChar = true;
      } else {
        if(!escapingChar && getCurrentChar() == '\'') {
          mOffset += 1;
          return;
        }
        escapingChar = false;
      }

      mOffset += 1;
    }
  }
  inline void consumeRegex() {
    assert(getCurrentChar() == '[');
    mOffset += 1;
    bool escapingChar = false;
    while(true) {
      if(mOffset >= mExprString.size()) {
        throw std::runtime_error{"Unterminated string in expression!"};
      }
      if(getCurrentChar() == '\\' && !escapingChar) {
        escapingChar = true;
      } else {
        if(!escapingChar && getCurrentChar() == ']') {
          mOffset += 1;
          return;
        }
        escapingChar = false;
      }

      mOffset += 1;
    }
  }
  inline char getCurrentChar() {
    return mExprString.at(mOffset);
  }
  inline bool skipWhitespaces() {
    const static std::string WHITESPACE_CHARS{"\t \n"};
    bool didSkip = false;
    while(mExprString.size() > mOffset && WHITESPACE_CHARS.find(mExprString.at(mOffset)) != std::string::npos) {
      didSkip = true;
      mOffset += 1;
    }
    return didSkip;
  }
  size_t mOffset = 0;
  const std::string_view mExprString;
  std::optional<std::string_view> mCurrentToken;
};

}