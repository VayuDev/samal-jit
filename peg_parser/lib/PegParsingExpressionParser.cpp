#include "peg_parser/PegParsingExpressionParser.hpp"
#include "peg_parser/PegUtil.hpp"

namespace peg {

static sp<ParsingExpression> parseChoice(ExpressionTokenizer &tok);

static sp<ParsingExpression> parseAtom(ExpressionTokenizer &tok) {
  if(!tok.currentToken())
    return nullptr;
  assert(!tok.currentToken()->empty());
  if(tok.currentToken()->at(0) == '\'') {
    auto expr = std::make_shared<TerminalParsingExpression>(std::string{tok.currentToken()->substr(1, tok.currentToken()->size() - 2)});
    tok.advance();
    return expr;
  } else if(*tok.currentToken() == "(") {
    tok.advance();
    auto expr = parseChoice(tok);
    if(!tok.currentToken()) {
      throw std::runtime_error{"Missing closing bracket"};
    }
    if(*tok.currentToken() != ")") {
      throw std::runtime_error{"Expected closing bracket, not '" + std::string{*tok.currentToken()}};
    }
    tok.advance();
    return expr;
  } else if(isalnum(tok.currentToken()->at(0))) {
    // non-terminal
    auto expr = std::make_shared<NonTerminalParsingExpression>(std::string{*tok.currentToken()});
    tok.advance();
    return expr;
  } else if(tok.currentToken()->at(0) == '[') {
    // regex
    auto expr = std::make_shared<TerminalParsingExpression>(std::string{*tok.currentToken()}, std::regex{"^" + std::string{*tok.currentToken()}});
    tok.advance();
    return expr;
  }
  throw std::runtime_error{std::string{"Invalid atomic token: '"} + std::string{*tok.currentToken()} + "'"};
}

static sp<ParsingExpression> parseQuantifier(ExpressionTokenizer &tok) {
  auto atom = parseAtom(tok);
  if(!tok.currentToken() || tok.currentToken()->empty()) {
    return atom;
  }
  if(*tok.currentToken() == "?") {
    auto expr = std::make_shared<OptionalParsingExpression>(std::move(atom));
    tok.advance();
    return expr;
  } else if(*tok.currentToken() == "*") {
    auto expr = std::make_shared<ZeroOrMoreParsingExpression>(std::move(atom));
    tok.advance();
    return expr;
  } else if(*tok.currentToken() == "+") {
    auto expr = std::make_shared<OneOrMoreParsingExpression>(std::move(atom));
    tok.advance();
    return expr;
  }
  return atom;
}

static sp<ParsingExpression> parseAttribute(ExpressionTokenizer &tok) {
  if(!tok.currentToken() || tok.currentToken()->empty()) {
    return nullptr;
  }
  if(*tok.currentToken() == "~sws~") {
    tok.advance();
    auto quantifier = parseQuantifier(tok);
    return std::make_shared<SkipWhitespacesExpression>(std::move(quantifier));
  }
  if(*tok.currentToken() == "~nws~") {
    tok.advance();
    auto quantifier = parseQuantifier(tok);
    return std::make_shared<DoNotSkipWhitespacesExpression>(std::move(quantifier));
  }
  return std::make_shared<SkipWhitespacesExpression>(parseQuantifier(tok));
}

static sp<ParsingExpression> parseErrorInfo(ExpressionTokenizer &tok) {
  auto attribute = parseAttribute(tok);
  if(!tok.currentToken() || tok.currentToken()->empty()) {
    return attribute;
  }
  if (tok.currentToken()->at(0) == '#') {
    // error-info
    auto expr = std::make_shared<ErrorMessageInfoExpression>(std::move(attribute), std::string{tok.currentToken()->substr(1, tok.currentToken()->size() - 2)});
    tok.advance();
    return expr;
  }
  return attribute;
}

static sp<ParsingExpression> parseSequence(ExpressionTokenizer &tok) {
  auto left = parseErrorInfo(tok);
  std::vector<sp<ParsingExpression>> children;
  children.emplace_back(std::move(left));
  while(tok.currentToken() && *tok.currentToken() != "|" && *tok.currentToken() != ")") {
    auto expr = parseErrorInfo(tok);
    if(!expr) {
      break;
    }
    children.emplace_back(expr);
  }
  if(children.size() == 1) {
    return std::move(children.at(0));
  }
  return std::make_shared<SequenceParsingExpression>(std::move(children));
}

static sp<ParsingExpression> parseChoice(ExpressionTokenizer &tok) {
  auto left = parseSequence(tok);
  std::vector<sp<ParsingExpression>> children;
  children.emplace_back(std::move(left));
  while(tok.currentToken() && *tok.currentToken() == "|") {
    tok.advance();
    children.emplace_back(parseSequence(tok));
  }
  if(children.size() == 1) {
    return std::move(children.at(0));
  }
  return std::make_shared<ChoiceParsingExpression>(std::move(children));
}

sp<ParsingExpression> stringToParsingExpression(const std::string_view &expr) {
  ExpressionTokenizer tok{expr};
  return parseChoice(tok);
}

ExpressionTokenizer::ExpressionTokenizer(const std::string_view& expr)
    : mExprString(expr) {
  genNextToken();
}

void ExpressionTokenizer::advance() {
  genNextToken();
}

void ExpressionTokenizer::genNextToken() {
  skipWhitespaces();
  auto start = mOffset;
  if(mOffset < mExprString.size()) {
    switch(getCurrentChar()) {
      case '\'':
        consumeString('\'', '\'');
        break;
      case '[':
        consumeString('[', ']');
        break;
      case '#':
        consumeString('#', '#');
        break;
      case '~':
        consumeString('~', '~');
        break;
      case '+':
      case '*':
      case ')':
      case '(':
      case '|':
      case '/':
      case '?':
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

void ExpressionTokenizer::consumeNonTerminal() {
  while(mOffset < mExprString.size() && isalnum(getCurrentChar())) {
    mOffset += 1;
  }
}

void ExpressionTokenizer::consumeString(const char start, const char end) {
  assert(getCurrentChar() == start);
  mOffset += 1;
  bool escapingChar = false;
  while(true) {
    if(mOffset >= mExprString.size()) {
      throw std::runtime_error{"Unterminated string in expression!"};
    }
    if(getCurrentChar() == '\\' && !escapingChar) {
      escapingChar = true;
    } else {
      if(!escapingChar && getCurrentChar() == end) {
        mOffset += 1;
        return;
      }
      escapingChar = false;
    }
    mOffset += 1;
  }
}

char ExpressionTokenizer::getCurrentChar() {
  return mExprString.at(mOffset);
}

bool ExpressionTokenizer::skipWhitespaces() {
  const static std::string WHITESPACE_CHARS{"\t \n"};
  bool didSkip = false;
  while(mExprString.size() > mOffset && WHITESPACE_CHARS.find(mExprString.at(mOffset)) != std::string::npos) {
    didSkip = true;
    mOffset += 1;
  }
  return didSkip;
}

}