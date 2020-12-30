#define CATCH_CONFIG_ENABLE_BENCHMARKING
#include <catch2/catch.hpp>
#include <peg_parser/PegParsingExpressionParser.hpp>
#include "peg_parser/PegTokenizer.hpp"
#include "peg_parser/PegParser.hpp"
#include <charconv>
#include <iostream>

TEST_CASE("Tokenizer matches strings", "[tokenizer]") {
  peg::PegTokenizer t{"a b c def"};
  peg::ParsingState state;
  auto shouldMatch = [&](const char* param) {
    auto ret = t.matchString(state, param);
    REQUIRE(ret);
    state = *ret;
    state = t.skipWhitespaces(state);
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
    state = t.skipWhitespaces(state);
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
    state = t.skipWhitespaces(state);
  };
  auto shouldMatch = [&](const char* param) {
    auto ret = t.matchString(state, param);
    REQUIRE(ret);
    state = *ret;
    state = t.skipWhitespaces(state);
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
  parseThenStringifyStaysSameAfter("'a' 'b'", "'a' 'b'");
  parseThenStringifyStaysSameAfter("'a' 'b' 'c'", "'a' 'b' 'c'");
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

TEST_CASE("ParsingExpression from string quantifiers attributes", "[parser]") {
  parseThenStringifyStaysSameAfter("~sws~'a'", "'a'");
  parseThenStringifyStaysSameAfter("~nws~'a' ~snn~'b'", "~nws~('a') ~snn~('b')");
  parseThenStringifyStaysSameAfter("~fws~'a' ~snn~'b'", "~fws~('a') ~snn~('b')");
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
    REQUIRE(parser.parse("Start", "abc").first.index() == 0);
    REQUIRE(parser.parse("Start", "a").first.index() == 1);
    REQUIRE(parser.parse("Start", "a b c").first.index() == 0);
    REQUIRE(parser.parse("Start", "ab").first.index() == 1);
  }
  {
    peg::PegParser parser;
    parser.addRule("Start", peg::stringToParsingExpression("'a' ~fws~ 'c'"));
    REQUIRE(parser.parse("Start", "a c").first.index() == 0);
    REQUIRE(parser.parse("Start", "a").first.index() == 1);
    REQUIRE(parser.parse("Start", "a b c").first.index() == 1);
    REQUIRE(parser.parse("Start", "ac").first.index() == 1);
  }
  {
    peg::PegParser parser;
    parser.addRule("Start", peg::stringToParsingExpression("'a' ('b' | 'c')"));
    REQUIRE(parser.parse("Start", "a b").first.index() == 0);
    REQUIRE(parser.parse("Start", "a c").first.index() == 0);
    REQUIRE(parser.parse("Start", "a").first.index() == 1);
    REQUIRE(parser.parse("Start", "a b c").first.index() == 1);
  }
  {
    peg::PegParser parser;
    parser.addRule("Start", peg::stringToParsingExpression("'a' ('b' | 'c' 'd') 'e'"));
    REQUIRE(parser.parse("Start", "a b e").first.index() == 0);
    REQUIRE(parser.parse("Start", "a c").first.index() == 1);
    REQUIRE(parser.parse("Start", "a e").first.index() == 1);
    REQUIRE(parser.parse("Start", "a cd e").first.index() == 0);
  }
}

TEST_CASE("Nonterminal parser match", "[parser]") {
  {
    peg::PegParser parser;
    parser.addRule("Start", peg::stringToParsingExpression("'a' | Second"));
    parser.addRule("Second", peg::stringToParsingExpression("'b' 'c'"));
    REQUIRE(parser.parse("Start", "a").first.index() == 0);
    REQUIRE(parser.parse("Start", "b c").first.index() == 0);
    REQUIRE(parser.parse("Start", "a b c").first.index() == 1);
  }
}

TEST_CASE("Quantifier parser match", "[parser]") {
  {
    peg::PegParser parser;
    parser.addRule("Start", peg::stringToParsingExpression("'a' | 'b' 'c'?"));
    REQUIRE(parser.parse("Start", "a").first.index() == 0);
    REQUIRE(parser.parse("Start", "b").first.index() == 0);
    REQUIRE(parser.parse("Start", "b c").first.index() == 0);
  }
  {
    peg::PegParser parser;
    parser.addRule("Start", peg::stringToParsingExpression("'a' | 'b' 'c'*"));
    REQUIRE(parser.parse("Start", "a").first.index() == 0);
    REQUIRE(parser.parse("Start", "b").first.index() == 0);
    REQUIRE(parser.parse("Start", "b c c c c").first.index() == 0);
  }
  {
    peg::PegParser parser;
    parser.addRule("Start", peg::stringToParsingExpression("'a' | 'b' 'c'+"));
    REQUIRE(parser.parse("Start", "a").first.index() == 0);
    REQUIRE(parser.parse("Start", "b").first.index() == 1);
    REQUIRE(parser.parse("Start", "b c c c c").first.index() == 0);
  }
}

TEST_CASE("Nonterminal parsing", "[parser]") {
  {
    peg::PegParser parser;
    parser.addRule("Start", peg::stringToParsingExpression("'a' | Second"), [](const peg::MatchInfo& i) {
      if(*i.choice == 0) {
        return 1;
      } else {
        return 2 + *i.subs.at(0).result.get<int*>();
      }
    });
    parser.addRule("Second", peg::stringToParsingExpression("'b' 'c'"), [](const peg::MatchInfo&) -> peg::Any {
      return 3;
    });
    REQUIRE(*std::get<0>(parser.parse("Start", "a").first).getMatchInfo().result.get<int*>() == 1);
    REQUIRE(*std::get<0>(parser.parse("Start", "b c").first).getMatchInfo().result.get<int*>() == 5);
  }
}
TEST_CASE("Calculator test", "[parser]") {
  peg::PegParser parser;
  parser.addRule("Expr", peg::stringToParsingExpression("Sum"), [](peg::MatchInfo& i) -> peg::Any {
    return std::move(i.result);
  });
  parser["Sum"] << "Product (('+' #Expected +# | '-' #Expected -#) Product)*" >> [](const peg::MatchInfo& i) -> peg::Any {
    int res = *i[0].result.get<int*>();
    for(auto& child: i[1].subs) {
      auto val = *child[1].result.get<int*>();
      if(*child[0].choice == 0) {
        res += val;
      } else {
        res -= val;
      }
    }
    return res;
  };
  parser["Product"] << "Value (('*' #Expected *# | '/' #Expected /#) Value)*" >> [](const peg::MatchInfo& i) -> peg::Any {
    int res = *i[0].result.get<int*>();
    for(auto& child: i[1].subs) {
      auto val = *child[1].result.get<int*>();
      if(*child[0].choice == 0) {
        res *= val;
      } else {
        res /= val;
      }
    }
    return res;
  };
  parser.addRule("Value", peg::stringToParsingExpression("[\\d]+ #Expected digit# | ('(' Expr ')') #Expected subexpression#"), [](peg::MatchInfo& i) -> peg::Any {
    int val;
    if(*i.choice == 0) {
      std::from_chars(i.startTrimmed(), i.endTrimmed(), val);
      return val;
    } else {
      return std::move(i[0][1].result);
    }
  });
  REQUIRE(*std::get<0>(parser.parse("Expr", "52 + 3").first).getMatchInfo().result.get<int*>() == 55);
  REQUIRE(*std::get<0>(parser.parse("Expr", "5 + 3*2").first).getMatchInfo().result.get<int*>() == 11);
  REQUIRE(*std::get<0>(parser.parse("Expr", "5*2 + 3").first).getMatchInfo().result.get<int*>() == 13);
  REQUIRE(*std::get<0>(parser.parse("Expr", "4/2 + 3*5").first).getMatchInfo().result.get<int*>() == 17);
  REQUIRE(*std::get<0>(parser.parse("Expr", "5-2*3").first).getMatchInfo().result.get<int*>() == -1);
  REQUIRE(*std::get<0>(parser.parse("Expr", "(5-2)*3").first).getMatchInfo().result.get<int*>() == 9);
  REQUIRE(*std::get<0>(parser.parse("Expr", "(20-2)*3").first).getMatchInfo().result.get<int*>() == 18*3);
  auto res = parser.parse("Expr", "3*hallo");
  //std::cout << peg::errorsToString(std::get<1>(res.first), res.second) << "\n";
  REQUIRE(peg::errorsToString(std::get<1>(res.first), res.second) != std::string{""});
}

TEST_CASE("Attribute parser match", "[parser]") {
  {
    peg::PegParser parser;
    parser["Start"] << "('a' ~snn~'\n')+";
    REQUIRE(parser.parse("Start", "a").first.index() == 1);
    REQUIRE(parser.parse("Start", "a\n").first.index() == 0);
    REQUIRE(parser.parse("Start", "a\na\n").first.index() == 0);
    REQUIRE(parser.parse("Start", "a  \n   a\n").first.index() == 0);
    REQUIRE(parser.parse("Start", "a \n a   ").first.index() == 1);
  }
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