#include "peg_parser/PegParsingExpressionParser.hpp"

namespace peg {

static sp<ParsingExpression> parseSequence(const std::string_view &expr) {

}

sp<ParsingExpression> stringToParsingExpression(const std::string_view &expr) {
  return parseSequence(expr);
}

}