#define CATCH_CONFIG_ENABLE_BENCHMARKING
#include "samal_lib/AST.hpp"
#include "samal_lib/Compiler.hpp"
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
    std::vector<samal::up<samal::ModuleRootNode>> modules;
    modules.emplace_back(std::move(ast.first));
    samal::Compiler comp{modules};
    auto program = comp.compile();
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
    auto vmRet = vm.run("Main.fib64", { samal::ExternalVMValue::wrapInt64(10) });
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
    auto vmRet = vm.run("Main.fib32", { samal::ExternalVMValue::wrapInt32(10) });
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
    auto vmRet = vm.run("Main.testTuple", std::vector<samal::ExternalVMValue>{});
    REQUIRE(vmRet.dump() == "{type: i32, value: 12}");
}

TEST_CASE("Ensure that lambda templates work", "[samal_whole_system]") {
    auto vm = compileSimple(R"(
fn test() -> i64 {
    lambda = makeLambda<i32>(5)
    lambda2 = makeLambda<i64>(24i64)
    lambda2(10i64)
}

fn makeLambda<T>(p : T) -> fn(T) -> T {
    fn(p2: T) -> T {
        p + p2
    }
})");
    auto vmRet = vm.run("Main.test", std::vector<samal::ExternalVMValue>{});
    REQUIRE(vmRet.dump() == "{type: i64, value: 34}");
}