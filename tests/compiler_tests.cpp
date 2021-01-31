#define CATCH_CONFIG_ENABLE_BENCHMARKING
#include "samal_lib/AST.hpp"
#include "samal_lib/Compiler.hpp"
#include "samal_lib/Parser.hpp"
#include <catch2/catch.hpp>
#include <charconv>
#include <iostream>

static void checkThatCompilationFails(const char* code) {
    samal::Parser parser;
    auto ast = parser.parse("Main", code);
    REQUIRE(ast.first);
    std::vector<samal::up<samal::ModuleRootNode>> modules;
    modules.emplace_back(std::move(ast.first));
    samal::Compiler comp{ modules, {} };
    REQUIRE_THROWS_AS(comp.compile(), samal::CompilationException);
}
static void checkThatCompilationSucceeds(const char* code) {
    samal::Parser parser;
    auto ast = parser.parse("Main", code);
    REQUIRE(ast.first);
    std::vector<samal::up<samal::ModuleRootNode>> modules;
    modules.emplace_back(std::move(ast.first));
    samal::Compiler comp{ modules, {} };
    comp.compile();
}

TEST_CASE("Ensure that we can't define a function twice", "[samal_type_completer]") {
    auto code = R"(
fn a(n : i32) -> i32 {
  n
}
fn a(a : i32, b : i32) -> i32 {
  a + b
})";
    checkThatCompilationFails(code);
}

TEST_CASE("Ensure that we don't mix types", "[samal_type_completer]") {
    auto code = R"(
fn a() -> i32 {
  x = 5 + a
})";
    checkThatCompilationFails(code);
}

TEST_CASE("Ensure that we check the parameters of function calls", "[samal_type_completer]") {
    {
        auto code = R"(
fn a(b : i32) -> i32 {
  x = a(a)
})";
        checkThatCompilationFails(code);
    }
    {
        auto code = R"(
fn a(b : i32) -> i32 {
  x = a()
})";
        checkThatCompilationFails(code);
    }
}

TEST_CASE("Ensure that normal cases work", "[samal_type_completer]") {
    {
        auto code = R"(
fn a(b : i32) -> i32 {
  x = a(b)
})";
        checkThatCompilationSucceeds(code);
    }
}

TEST_CASE("Chained function calls", "[samal_type_completer]") {
    {
        auto code = R"(
fn a(p : i32) -> i32 {
  x = b(5)(3)
}
fn b(p : i32) -> fn(i32) -> i32 {
  a
})";
        checkThatCompilationSucceeds(code);
    }
}

TEST_CASE("Empty code", "[samal_type_completer]") {
    {
        samal::Parser parser;
        auto code = R"()";
        auto ast = parser.parse("Main", code);
        REQUIRE(!ast.first);
    }
    {
        samal::Parser parser;
        auto code = R"(fn a(p: i32) -> i32 { 5
 }  )";
        auto ast = parser.parse("Main", code);
        REQUIRE(ast.first);
    }
}

TEST_CASE("Tuple function calls", "[samal_type_completer]") {
    {
        auto code = R"(
fn a(p : i32) -> i32 {
  x = (5, 3)
  b(x)
}
fn b(p : (i32, i32)) -> i32 {
  0
})";
        checkThatCompilationSucceeds(code);
    }
    {
        auto code = R"(
fn a(p : i32) -> () {
  b((5, 3))
  ()
}
fn b(p : (i32, i32)) -> i32 {
  0
})";
        checkThatCompilationSucceeds(code);
    }
}

TEST_CASE("Correct function return types checking", "[samal_type_completer]") {
    {
        auto code = R"(
fn a(p : i32) -> i32 {
  if p > 5 {
    p
  } else {
    0
  }
})";
        checkThatCompilationSucceeds(code);
    }
    {
        auto code = R"(
fn a(p : i32) -> i32 {
  if p > 5 {
    (p, 5)
  } else {
    0
  }
})";
        checkThatCompilationFails(code);
    }
}