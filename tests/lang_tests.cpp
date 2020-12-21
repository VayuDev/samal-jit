#define CATCH_CONFIG_ENABLE_BENCHMARKING
#include <catch2/catch.hpp>
#include <charconv>
#include <iostream>
#include "samal_lib/Parser.hpp"
#include "samal_lib/AST.hpp"
#include "samal_lib/DatatypeCompleter.hpp"

TEST_CASE("Ensure that we can't define a function twice", "[samal_type_completer]") {
  samal::Parser parser;
  auto code = R"(
fn a(n : i32) -> i32 {
  a;
}
fn a(a : i32, b : i32) -> i32 {
  a + b;
})";
  auto ast = parser.parse(code);
  REQUIRE(ast);
  samal::DatatypeCompleter completer;
  std::vector<samal::up<samal::ModuleRootNode>> modules;
  modules.emplace_back(std::move(ast));
  REQUIRE_THROWS(completer.declareModules(modules));
}