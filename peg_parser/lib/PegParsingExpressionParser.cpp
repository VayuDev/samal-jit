#include "peg_parser/PegParsingExpressionParser.hpp"

namespace peg {


static sp<ParsingExpression> parseSequence(ExpressionTokenizer &expr) {

}

static sp<ParsingExpression> parseChoice(ExpressionTokenizer &expr) {
  auto left = parseSequence(expr);

}

sp<ParsingExpression> stringToParsingExpression(const std::string_view &expr) {
  ExpressionTokenizer tok{expr};
  return parseChoice(tok);
}

}