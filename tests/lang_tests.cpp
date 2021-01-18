#define CATCH_CONFIG_ENABLE_BENCHMARKING
#include "samal_lib/AST.hpp"
#include "samal_lib/Compiler.hpp"
#include "samal_lib/DatatypeCompleter.hpp"
#include "samal_lib/ExternalVMValue.hpp"
#include "samal_lib/Parser.hpp"
#include "samal_lib/VM.hpp"
#include <catch2/catch.hpp>
#include <charconv>
#include <iostream>

samal::VM compileSimple(const char* code) {
    samal::Parser parser;
    auto ast = parser.parse("Main", code);
    REQUIRE(ast.first);
    samal::DatatypeCompleter completer;
    std::vector<samal::up<samal::ModuleRootNode>> modules;
    modules.emplace_back(std::move(ast.first));
    completer.declareModules(modules);
    completer.complete(modules.at(0));
    samal::Compiler comp;
    auto program = comp.compile(modules);
    return samal::VM{ std::move(program) };
}

TEST_CASE("Ensure that fib64 works", "[samal_whole_system]") {
    auto vm = compileSimple(R"(
fn fib64(n : i64) -> i64 {
    if n < 2i64 {
        n
    } else {
        fib64(n - 1i64) + fib64(n - 2i64)
    }
})");
    auto vmRet = vm.run("fib64", { samal::ExternalVMValue::wrapInt64(10) });
    REQUIRE(vmRet.dump() == "{type: i64, value: 55}");
}

TEST_CASE("Ensure that fib32 works", "[samal_whole_system]") {
    auto vm = compileSimple(R"(
fn fib32(n : i32) -> i32 {
    if n < 2 {
        n
    } else {
        fib32(n - 1) + fib32(n - 2)
    }
})");
    auto vmRet = vm.run("fib32", { samal::ExternalVMValue::wrapInt32(10) });
    REQUIRE(vmRet.dump() == "{type: i32, value: 55}");
}

TEST_CASE("Ensure that tuples work", "[samal_whole_system]") {
    auto vm = compileSimple(R"(
fn join(a : i32, b : i64) -> (i32, i64) {
    (a, b + 5i64)
}
fn testTuple() -> i32 {
    t = (join(5, 1i64), 2)
    t1 = t:0
    t2 = t:1
    t:0:0 + t2 + t1:0
})");
    auto vmRet = vm.run("testTuple", std::vector<samal::ExternalVMValue>{});
    REQUIRE(vmRet.dump() == "{type: i32, value: 12}");
}