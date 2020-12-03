#include <catch2/catch.hpp>
#include <peg_parser/PegParsingExpressionParser.hpp>
#include "peg_parser/PegTokenizer.hpp"
#include "peg_parser/PegParser.hpp"

TEST_CASE("Tokenizer matches strings", "[tokenizer]") {
  peg::PegTokenizer t{"a b c def"};
  t.pushState();
  REQUIRE(t.matchString("a"));
  REQUIRE(t.matchString("b"));
  REQUIRE(t.matchString("c"));
  REQUIRE(!t.isEmpty());
  t.popState();
  REQUIRE(t.matchString("a"));
  REQUIRE(t.matchString("b"));
  REQUIRE(!t.matchString("cd"));
  REQUIRE(!t.matchString("eghh"));
  REQUIRE(t.matchString("c"));
  REQUIRE(t.matchString("de"));
  REQUIRE(!t.matchString("de"));
  REQUIRE(t.matchString("f"));
  REQUIRE(t.isEmpty());
}

TEST_CASE("Tokenizer matches regex", "[tokenizer]") {
  peg::PegTokenizer t{"a b c def"};
  t.pushState();
  REQUIRE(t.matchRegex(std::regex{"^."}));
  REQUIRE(t.matchString("b"));
  REQUIRE(t.matchRegex(std::regex{"^c"}));
  REQUIRE(!t.matchRegex(std::regex{"^f"}));
}

TEST_CASE("ParsingExpression stringify", "[parser]") {
  auto rule = std::make_shared<peg::SequenceParsingExpression>(std::vector<peg::sp<peg::ParsingExpression>>{
      std::make_shared<peg::TerminalParsingExpression>("a"),
      std::make_shared<peg::ChoiceParsingExpression>(std::vector<peg::sp<peg::ParsingExpression>>{
          std::make_shared<peg::TerminalParsingExpression>("c"),
          std::make_shared<peg::TerminalParsingExpression>("d")
      }),
      std::make_shared<peg::TerminalParsingExpression>("b")
  });
  REQUIRE(rule->dump() == "'a' ('c' | 'd') 'b'");
}

TEST_CASE("ParsingExpression from string", "[parser]") {
  auto parseStringifyStaysSame = [] (const char* str) {
    REQUIRE(str == peg::stringToParsingExpression(str)->dump());
  };
  parseStringifyStaysSame("'a' ('c' | 'd') 'b'");
}