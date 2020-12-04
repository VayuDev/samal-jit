#include "peg_parser/PegParsingExpressionParser.hpp"
#include "peg_parser/PegUtil.hpp"

namespace peg {

static sp<ParsingExpression> parseChoice(ExpressionTokenizer &tok);

static sp<ParsingExpression> parseAtom(ExpressionTokenizer &tok) {
  if(!tok.currentToken())
    return nullptr;
  if(tok.currentToken()->find("'") == 0) {
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
  }
  throw std::runtime_error{std::string{"Invalid atomic token: '"} + std::string{*tok.currentToken()} + "'"};
}


static sp<ParsingExpression> parseSequence(ExpressionTokenizer &tok) {
  auto left = parseAtom(tok);
  std::vector<sp<ParsingExpression>> children;
  children.emplace_back(std::move(left));
  while(tok.currentToken() && *tok.currentToken() != "|" && *tok.currentToken() != ")") {
    auto expr = parseAtom(tok);
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

}