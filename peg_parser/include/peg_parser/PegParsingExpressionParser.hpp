#pragma once
#include "peg_parser/PegParser.hpp"

namespace peg {

sp<ParsingExpression> stringToParsingExpression(const std::string_view &);

}