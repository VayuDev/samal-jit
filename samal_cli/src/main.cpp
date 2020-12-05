#include <iostream>
#include "peg_parser/PegParser.hpp"
#include "peg_parser/PegParsingExpression.hpp"

int main() {
  std::cout << sizeof(peg::RuleResult) << "\n";
}