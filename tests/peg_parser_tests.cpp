#include <catch2/catch.hpp>
#include <peg_parser/PegParsingExpressionParser.hpp>
#include "peg_parser/PegTokenizer.hpp"
#include "peg_parser/PegParser.hpp"

TEST_CASE("Tokenizer matches strings", "[tokenizer]") {
  peg::PegTokenizer t{"a b c def"};
  peg::ParsingState state;
  auto shouldMatch = [&](const char* param) {
    auto ret = t.matchString(state, param);
    REQUIRE(ret);
    state = *ret;
  };
  auto shouldNotMatch = [&](const char* param) {
    auto ret = t.matchString(state, param);
    REQUIRE(!ret);
  };
  shouldMatch("a");
  shouldMatch("b");
  shouldMatch("c");
  shouldNotMatch("abc");
  state = peg::ParsingState{};
  shouldMatch("a");
  shouldMatch("b");
  shouldMatch("c");
  shouldMatch("de");
  shouldNotMatch("gef");
  shouldMatch("f");
}

TEST_CASE("Tokenizer matches regex", "[tokenizer]") {
  peg::PegTokenizer t{"a b c def"};
  peg::ParsingState state;
  auto shouldMatchReg = [&](const char* param) {
    auto ret = t.matchRegex(state, std::regex{param});
    REQUIRE(ret);
    state = *ret;
  };
  auto shouldNotMatchReg = [&](const char* param) {
    auto ret = t.matchRegex(state, std::regex{param});
    REQUIRE(!ret);
  };
  shouldMatchReg("^.");
  shouldMatchReg("^b");
  shouldMatchReg("^c");
  shouldNotMatchReg("^f");
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

TEST_CASE("ExpressionTokenizer simple", "[parser]") {
  std::string s = "( that is) gre+at*";
  peg::ExpressionTokenizer tokenizer{s};
  REQUIRE(*tokenizer.currentToken() == "(");
  tokenizer.advance();
  REQUIRE(*tokenizer.currentToken() == "that");
  tokenizer.advance();
  REQUIRE(*tokenizer.currentToken() == "is");
  tokenizer.advance();
  REQUIRE(*tokenizer.currentToken() == ")");
  tokenizer.advance();
  REQUIRE(*tokenizer.currentToken() == "gre");
  tokenizer.advance();
  REQUIRE(*tokenizer.currentToken() == "+");
  tokenizer.advance();
  REQUIRE(*tokenizer.currentToken() == "at");
  tokenizer.advance();
  REQUIRE(*tokenizer.currentToken() == "*");
  tokenizer.advance();
  REQUIRE(!tokenizer.currentToken().has_value());
  tokenizer.advance();
  REQUIRE(!tokenizer.currentToken().has_value());
  tokenizer.advance();
  REQUIRE(!tokenizer.currentToken().has_value());
}

TEST_CASE("ExpressionTokenizer string and regex", "[parser]") {
  std::string s = "'strings \\'are' [so great and] nice";
  peg::ExpressionTokenizer tokenizer{s};
  REQUIRE(*tokenizer.currentToken() == "'strings \\'are'");
  tokenizer.advance();
  REQUIRE(*tokenizer.currentToken() == "[so great and]");
  tokenizer.advance();
  REQUIRE(*tokenizer.currentToken() == "nice");
  tokenizer.advance();
}

TEST_CASE("ExpressionTokenizer string and regex escapes", "[parser]") {
  std::string s = R"('strings \\' [so great and] nice)";
  peg::ExpressionTokenizer tokenizer{s};
  REQUIRE(*tokenizer.currentToken() == R"('strings \\')");
  tokenizer.advance();
  REQUIRE(*tokenizer.currentToken() == "[so great and]");
  tokenizer.advance();
  REQUIRE(*tokenizer.currentToken() == "nice");
  tokenizer.advance();
}

TEST_CASE("ExpressionTokenizer advanced", "[parser]") {
  std::string s = "'a' ('b' | 'c')";
  peg::ExpressionTokenizer tokenizer{s};
  REQUIRE(*tokenizer.currentToken() == "'a'");
  tokenizer.advance();
  REQUIRE(*tokenizer.currentToken() == "(");
  tokenizer.advance();
  REQUIRE(*tokenizer.currentToken() == "'b'");
  tokenizer.advance();
  REQUIRE(*tokenizer.currentToken() == "|");
  tokenizer.advance();
  REQUIRE(*tokenizer.currentToken() == "'c'");
  tokenizer.advance();
  REQUIRE(*tokenizer.currentToken() == ")");
  tokenizer.advance();
}

auto parseThenStringifyStaysSame = [] (const char* str) {
  auto res = peg::stringToParsingExpression(str);
  REQUIRE(res);
  REQUIRE(str == res->dump());
};
auto parseThenStringifyStaysSameAfter = [] (const char* str, const char* after) {
  auto res = peg::stringToParsingExpression(str);
  REQUIRE(res);
  REQUIRE(after == res->dump());
};

TEST_CASE("ParsingExpression from string simple", "[parser]") {
  parseThenStringifyStaysSame("'a' 'b'");
  parseThenStringifyStaysSame("'a' 'b' 'c'");
  parseThenStringifyStaysSameAfter("'a' 'b' 'c' | 'd'", "('a' 'b' 'c' | 'd')");
  parseThenStringifyStaysSame("'a' ('b' | 'c')");
  parseThenStringifyStaysSame("'a' ('b' | 'c') 'd'");
}

TEST_CASE("ParsingExpression from string quantifiers", "[parser]") {
  parseThenStringifyStaysSameAfter("'a'+", "('a')+");
  parseThenStringifyStaysSameAfter("'a'? 'b' 'c'*", "('a')? 'b' ('c')*");
  parseThenStringifyStaysSame("('a')? ('b' 'c')*");
}