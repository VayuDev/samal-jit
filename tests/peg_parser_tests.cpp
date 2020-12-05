#define CATCH_CONFIG_ENABLE_BENCHMARKING
#include <catch2/catch.hpp>
#include <peg_parser/PegParsingExpressionParser.hpp>
#include "peg_parser/PegTokenizer.hpp"
#include "peg_parser/PegParser.hpp"
#include <charconv>

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
  state = peg::ParsingState{};
  shouldMatch("a");
  shouldMatch("b");
  shouldMatch("c");
  shouldMatch("def");
  shouldNotMatch("a");
  shouldNotMatch("f");
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

TEST_CASE("Tokenizer matches regex 2", "[tokenizer]") {
  peg::PegTokenizer t{"5 + 3"};
  peg::ParsingState state;
  auto shouldMatchReg = [&](const char* param) {
    auto ret = t.matchRegex(state, std::regex{param});
    REQUIRE(ret);
    state = *ret;
  };
  auto shouldMatch = [&](const char* param) {
    auto ret = t.matchString(state, param);
    REQUIRE(ret);
    state = *ret;
  };
  shouldMatchReg("^[\\d]");
  shouldMatch("+");
  shouldMatchReg("^[\\d]");
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
  parseThenStringifyStaysSame("Product (('+' | '-') Product)*");
  parseThenStringifyStaysSame("Value");
}

TEST_CASE("ParsingExpression from string regex", "[parser]") {
  parseThenStringifyStaysSameAfter("[\\d]", "[\\d]");
}

#ifdef SAMAL_PEG_PARSER_BENCHMARKS
TEST_CASE("ParsingExpression conversion benchmarks", "[parser]") {
  BENCHMARK("Expression compilation 1") {
    return peg::stringToParsingExpression("'a' ('b' | 'c' 'd') 'e'");
  };
  BENCHMARK("Expression compilation 2") {
    return peg::stringToParsingExpression("'a' 'abcdef' 'd' | 'f' 'g' ('e' | '2')");
  };
}
#endif

TEST_CASE("Simple parser match", "[parser]") {
  {
    peg::PegParser parser;
    parser.addRule("Start", peg::stringToParsingExpression("'a' 'b' 'c'"));
    REQUIRE(parser.parse("Start", "abc").index() == 0);
    REQUIRE(parser.parse("Start", "a").index() == 1);
    REQUIRE(parser.parse("Start", "a b c").index() == 0);
    REQUIRE(parser.parse("Start", "ab").index() == 1);
  }
  {
    peg::PegParser parser;
    parser.addRule("Start", peg::stringToParsingExpression("'a' ' ' 'c'"));
    REQUIRE(parser.parse("Start", "a c").index() == 0);
    REQUIRE(parser.parse("Start", "a").index() == 1);
    REQUIRE(parser.parse("Start", "a b c").index() == 1);
    REQUIRE(parser.parse("Start", "ac").index() == 1);
  }
  {
    peg::PegParser parser;
    parser.addRule("Start", peg::stringToParsingExpression("'a' ('b' | 'c')"));
    REQUIRE(parser.parse("Start", "a b").index() == 0);
    REQUIRE(parser.parse("Start", "a c").index() == 0);
    REQUIRE(parser.parse("Start", "a").index() == 1);
    REQUIRE(parser.parse("Start", "a b c").index() == 1);
  }
  {
    peg::PegParser parser;
    parser.addRule("Start", peg::stringToParsingExpression("'a' ('b' | 'c' 'd') 'e'"));
    REQUIRE(parser.parse("Start", "a b e").index() == 0);
    REQUIRE(parser.parse("Start", "a c").index() == 1);
    REQUIRE(parser.parse("Start", "a e").index() == 1);
    REQUIRE(parser.parse("Start", "a cd e").index() == 0);
  }
}

TEST_CASE("Nonterminal parser match", "[parser]") {
  {
    peg::PegParser parser;
    parser.addRule("Start", peg::stringToParsingExpression("'a' | Second"));
    parser.addRule("Second", peg::stringToParsingExpression("'b' 'c'"));
    REQUIRE(parser.parse("Start", "a").index() == 0);
    REQUIRE(parser.parse("Start", "b c").index() == 0);
    REQUIRE(parser.parse("Start", "a b c").index() == 1);
  }
}

TEST_CASE("Quantifier parser match", "[parser]") {
  {
    peg::PegParser parser;
    parser.addRule("Start", peg::stringToParsingExpression("'a' | 'b' 'c'?"));
    REQUIRE(parser.parse("Start", "a").index() == 0);
    REQUIRE(parser.parse("Start", "b").index() == 0);
    REQUIRE(parser.parse("Start", "b c").index() == 0);
  }
  {
    peg::PegParser parser;
    parser.addRule("Start", peg::stringToParsingExpression("'a' | 'b' 'c'*"));
    REQUIRE(parser.parse("Start", "a").index() == 0);
    REQUIRE(parser.parse("Start", "b").index() == 0);
    REQUIRE(parser.parse("Start", "b c c c c").index() == 0);
  }
  {
    peg::PegParser parser;
    parser.addRule("Start", peg::stringToParsingExpression("'a' | 'b' 'c'+"));
    REQUIRE(parser.parse("Start", "a").index() == 0);
    REQUIRE(parser.parse("Start", "b").index() == 1);
    REQUIRE(parser.parse("Start", "b c c c c").index() == 0);
  }
}

TEST_CASE("Nonterminal parsing", "[parser]") {
  {
    peg::PegParser parser;
    parser.addRule("Start", peg::stringToParsingExpression("'a' | Second"), [](const peg::MatchInfo& i) -> std::any {
      if(*i.choice == 0) {
        return 1;
      } else {
        return 2 + std::any_cast<int>(i.subs.at(0).result);
      }
    });
    parser.addRule("Second", peg::stringToParsingExpression("'b' 'c'"), [](const peg::MatchInfo&) -> std::any {
      return 3;
    });
    REQUIRE(std::any_cast<int>(std::get<0>(parser.parse("Start", "a")).second.result) == 1);
    REQUIRE(std::any_cast<int>(std::get<0>(parser.parse("Start", "b c")).second.result) == 5);
  }
}
TEST_CASE("Calculator test", "[parser]") {
  peg::PegParser parser;
  parser.addRule("Expr", peg::stringToParsingExpression("Sum"), [](const peg::MatchInfo& i) -> std::any {
    return i.result;
  });
  parser.addRule("Sum", peg::stringToParsingExpression("Product (('+' | '-') Product)*"), [](const peg::MatchInfo& i) -> std::any {
    int res = std::any_cast<int>(i.subs.at(0).result);
    for(auto& child: i.subs.at(1).subs) {
      auto val = std::any_cast<int>(child.subs.at(1).result);
      if(*child.subs.at(0).choice == 0) {
        res += val;
      } else {
        res -= val;
      }
    }
    return res;
  });
  parser.addRule("Product", peg::stringToParsingExpression("Value (('*' | '/') Value)*"), [](const peg::MatchInfo& i) -> std::any {
    int res = std::any_cast<int>(i.subs.at(0).result);
    for(auto& child: i.subs.at(1).subs) {
      auto val = std::any_cast<int>(child.subs.at(1).result);
      if(*child.subs.at(0).choice == 0) {
        res *= val;
      } else {
        res /= val;
      }
    }
    return res;
  });
  parser.addRule("Value", peg::stringToParsingExpression("[\\d]+ | ('(' Expr ')')"), [](const peg::MatchInfo& i) -> std::any {
    int val;
    if(*i.choice == 0) {
      // trim string
      auto start = i.start;
      auto end = i.end;
      while(*start == ' ' && start < end) {
        start += 1;
      }
      while(*(end-1) == ' ' && start < end) {
        end -= 1;
      }
      std::from_chars(start, end, val);
      return val;
    } else {
      return i.subs.at(0).result;
    }
  });
  REQUIRE(std::any_cast<int>(std::get<0>(parser.parse("Expr", "5 + 3")).second.result) == 8);
  REQUIRE(std::any_cast<int>(std::get<0>(parser.parse("Expr", "5 + 3*2")).second.result) == 11);
  REQUIRE(std::any_cast<int>(std::get<0>(parser.parse("Expr", "5*2 + 3")).second.result) == 13);
  REQUIRE(std::any_cast<int>(std::get<0>(parser.parse("Expr", "4/2 + 3*5")).second.result) == 17);
  REQUIRE(std::any_cast<int>(std::get<0>(parser.parse("Expr", "5-2*3")).second.result) == -1);

}
#ifdef SAMAL_PEG_PARSER_BENCHMARKS
TEST_CASE("ParsingExpression matching benchmarks", "[parser]") {
  peg::PegParser parser;
  parser.addRule("Start", peg::stringToParsingExpression("'a' ('b' | 'c' 'd') 'e'"));
  BENCHMARK("Expression matching") {
    return parser.parse("Start", "a cd e");
  };
}
#endif