#define CATCH_CONFIG_ENABLE_BENCHMARKING
#include "samal_lib/AST.hpp"
#include "samal_lib/Compiler.hpp"
#include "samal_lib/ExternalVMValue.hpp"
#include "samal_lib/Parser.hpp"
#include "samal_lib/VM.hpp"
#include <catch2/catch.hpp>
#include <charconv>
#include <iostream>

samal::VM compileSimple(const char* code, samal::VMParameters params = {}) {
    samal::Parser parser;
    auto ast = parser.parse("Main", code);
    REQUIRE(ast.first);
    std::vector<samal::up<samal::ModuleRootNode>> modules;
    modules.emplace_back(std::move(ast.first));
    samal::Compiler comp{ modules, {} };
    auto program = comp.compile();
    return samal::VM{ std::move(program), params };
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
    auto vmRet = vm.run("Main.fib64", { samal::ExternalVMValue::wrapInt64(vm, 10) });
    REQUIRE(vmRet.dump() == "55i64");
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
    auto vmRet = vm.run("Main.fib32", { samal::ExternalVMValue::wrapInt32(vm, 10) });
    REQUIRE(vmRet.dump() == "55");
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
    REQUIRE(vmRet.dump() == "12");
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
    REQUIRE(vmRet.dump() == "34i64");
}

TEST_CASE("Ensure that lists work", "[samal_whole_system]") {
    auto vm = compileSimple(R"(
fn len<T>(l : [T]) -> i32 {
    if l == [] {
        0
    } else {
        1 + len<T>(l:tail)
    }
}

fn concat<T>(l1 : [T], l2 : [T]) -> [T] {
    if l1 == [] {
        l2
    } else {
        l1:head + concat<T>(l1:tail, l2)
    }
}

fn map<T, S>(l : [T], callback : fn(T) -> S) -> [S] {
    if l == [] {
        [:S]
    } else {
        callback(l:head) + map<T, S>(l:tail, callback)
    }
}


fn seqRec<T>(index : T, limit: T) -> [T] {
    if limit < 1 {
        [:T]
    } else {
        index + seqRec<T>(index + 1, limit - 1)
    }
}

fn seq<T>(limit: T) -> [T] {
    seqRec<T>(0, limit)
}

fn test() -> [i32] {
    l1 = seq<i32>(3)
    l2 = seq<i32>(3)
    combined = concat<i32>(l1, l2)
    lambda = fn(p2 : i32) -> i32 {
        p2 + len<i32>(l1)
    }
    added = map<i32, i32>(combined, lambda)
    added
})");
    auto vmRet = vm.run("Main.test", std::vector<samal::ExternalVMValue>{});
    REQUIRE(vmRet.dump() == "[3, 4, 5, 3, 4, 5]");
}
TEST_CASE("Euler problem 1", "[samal_whole_system]") {
    auto vm = compileSimple(R"(
fn sumRec<T>(current : T, list : [T]) -> T {
    if list == [] {
        current
    } else {
        @tail_call_self(current + list:head, list:tail)
    }
}
fn sum<T>(list : [T]) -> T {
    sumRec<T>(0, list)
}

fn filter<T>(list : [T], condition : fn(T) -> bool) -> [T] {
    if list == [] {
        [:T]
    } else {
        head = list:head
        if condition(head) {
            head + filter<T>(list:tail, condition)
        } else {
            filter<T>(list:tail, condition)
        }
    }
}

fn seqRec<T>(index : T, limit: T) -> [T] {
    if limit < 1 {
        [:T]
    } else {
        index + seqRec<T>(index + 1, limit - 1)
    }
}

fn seq<T>(limit: T) -> [T] {
    seqRec<T>(0, limit)
}

fn problem1() -> i32 {
    seq<i32>(10)
    |> filter<i32>(fn(i : i32) -> bool {
        i % 5 == 0 || i % 3 == 0
    })
    |> sum<i32>()
})");
    auto vmRet = vm.run("Main.problem1", std::vector<samal::ExternalVMValue>{});
    REQUIRE(vmRet.dump() == "23");
}

TEST_CASE("Ensure that tail recursion works", "[samal_whole_system]") {
    auto vm = compileSimple(R"(
fn test(n : i32) -> i32 {
    if n > 1000 {
        5
    } else {
        @tail_call_self(n + 1)
    }
})");
    auto vmRet = vm.run("Main.test", std::vector<samal::ExternalVMValue>{ samal::ExternalVMValue::wrapInt32(vm, 0) });
    REQUIRE(vmRet.dump() == "5");
}

TEST_CASE("Ensure that structs work", "[samal_whole_system]") {
    auto vm = compileSimple(R"(
struct Vec2 {
    x : i64,
    y : i32
}

fn test() -> (i32, Vec2) {
    (Vec2{x : 5i64, y : 42}:y, Vec2{x : 3i64, y : 5})
})");
    auto vmRet = vm.run("Main.test", std::vector<samal::ExternalVMValue>{ });
    REQUIRE(vmRet.dump() == "(42, Main.Vec2{x: 3i64, y: 5})");
}

TEST_CASE("Ensure that structs work advanced", "[samal_whole_system]") {
    auto vm = compileSimple(R"(
struct Point<T> {
    x : T,
    y : T
}

fn templ<T>() -> i32 {
    point = Point<T>{x : 5, y : 3}
    point:y
}

fn makePoint<T>(a : T, b : T) -> fn() -> Point<T> {
    fn() -> Point<T> {
        Point<T>{x : a, y : b}
    }
}

fn test() -> i32 {
    bp = makePoint<bool>(true, false)()
    if bp:x {
        templ<i32>()
    } else {
        3329
    }
})");
    auto vmRet = vm.run("Main.test", std::vector<samal::ExternalVMValue>{ });
    REQUIRE(vmRet.dump() == "3");
}

TEST_CASE("Ensure that we can create enums", "[samal_whole_system]") {
    auto vm = compileSimple(R"(
enum Shape {
    Circle{i32},
    Rectangle{i32, i32}
}
fn test() -> Shape {
   Shape::Rectangle{5, 3}
})");
    auto vmRet = vm.run("Main.test", std::vector<samal::ExternalVMValue>{ });
    REQUIRE(vmRet.dump() == "Main.Shape::Rectangle{5, 3}");
}

TEST_CASE("Test simple recursive match", "[samal_whole_system]") {
    auto vm = compileSimple(R"(
struct Vec2 {
    x : i32,
    y : i32
}

enum B {
    Opt1{[i32]},
    Opt2{i32, Vec2}
}

enum A {
    Opt1{B},
    Opt2{}
}

fn test() -> (i32, i32) {
    t = A::Opt1{B::Opt2{3, Vec2{x:71, y:72}}}
    t2 = A::Opt1{B::Opt1{[1, 2, 3]}}

    myMatch = fn(t : A) -> i32 {
        match t {
            Opt2{} -> 0,
            Opt1{Opt1{list}} -> 6 + list:head,
            Opt1{Opt2{a, b}} -> {
                b:x
            }
        }
    }
    x1 = myMatch(t)
    x2 = myMatch(t2)
    (x1, x2)
})");
    auto vmRet = vm.run("Main.test", std::vector<samal::ExternalVMValue>{ });
    REQUIRE(vmRet.dump() == "(71, 7)");
}

TEST_CASE("Ensure that we correctly check recursive types (user type)", "[samal_whole_system]") {
    auto vm = compileSimple(R"(
enum Tree {
    Branch{$Tree, i32, $Tree},
    None{}
}
fn test() -> Tree {
   Tree::Branch{$Tree::None{}, 5, $Tree::None{}}
})");
    auto vmRet = vm.run("Main.test", std::vector<samal::ExternalVMValue>{ });
    REQUIRE(vmRet.dump() == "Main.Tree::Branch{$Main.Tree::None{}, 5, $Main.Tree::None{}}");
}

TEST_CASE("Ensure that we correctly check recursive types (template type)", "[samal_whole_system]") {
    REQUIRE_THROWS(compileSimple(R"(
enum Maybe<T> {
    Some{T},
    None{}
}
fn test() -> Maybe<$i64> {
   Maybe<i64>::Some{5i64}
})"));
}

TEST_CASE("Weird pointer enum test", "[samal_whole_system]") {
    auto vm = compileSimple(R"(
struct Vec2 {
    x : [i32],
    y : i32
}

enum Maybe<T> {
    Some{T},
    None{}
}

fn test() -> (Vec2, [i32], i32, Maybe<Maybe<$i32>>, $i32) {
    v = Vec2{x : [1, 2, 3], y : 3}
    x = Maybe<Maybe<$i32>>::Some{Maybe<$i32>::Some{$5}}
    n = match x {
        Some{None{}} -> $0,
        Some{Some{n}} -> n,
        None{} -> $0
    }
    f = fn() -> () {
        ()
    }
    f()
    (v, v:x, v:y, x, n)
})");
    auto vmRet = vm.run("Main.test", std::vector<samal::ExternalVMValue>{ });
    REQUIRE(vmRet.dump() == "(Main.Vec2{x: [1, 2, 3], y: 3}, [1, 2, 3], 3, Main.Maybe::Some{Main.Maybe::Some{$5}}, $5)");
}
TEST_CASE("Modulo and div", "[samal_whole_system]") {
    auto vm = compileSimple(R"(
fn test() -> (i32, i64) {
    ((5 % 3 + 13) / 2, (5i64 % 3i64 + 13i64) / 2i64)
})");
    auto vmRet = vm.run("Main.test", std::vector<samal::ExternalVMValue>{ });
    REQUIRE(vmRet.dump() == "(7, 7i64)");
}

TEST_CASE("Character support", "[samal_whole_system]") {
    auto vm = compileSimple(R"(
fn test() -> [[char]] {
    ["Hallö \"von\" €😀", "Tom"]
})");
    auto vmRet = vm.run("Main.test", std::vector<samal::ExternalVMValue>{ });
    REQUIRE(vmRet.dump() == R"(["Hallö "von" €😀", "Tom"])");
}

TEST_CASE("Function template type inference", "[samal_whole_system]") {
    auto vm = compileSimple(R"(
fn add<T>(a : T, b : T) -> T {
    a + b
}

fn makeLambda<T>(p : T) -> fn(T) -> T {
    fn(p2: T) -> T {
        p + p2
    }
}
fn test() -> (i32, i64, i32) {
    a = add(2, 5)
    b =
        3i64
        |> add(5i64)

    c = 2 |> makeLambda()


    (a, b, c(3))
})");
    auto vmRet = vm.run("Main.test", std::vector<samal::ExternalVMValue>{ });
    REQUIRE(vmRet.dump() == R"((7, 8i64, 5))");
}

TEST_CASE("GC doesn't collect our parameters early for chained function calls", "[samal_whole_system]") {
    auto vm = compileSimple(R"(
fn identity<T>(p: T) -> T {
    p
}
fn concat<T>(l1 : [T], l2 : [T]) -> [T] {
    if l1 == [] {
        l2
    } else {
        l1:head + concat<T>(l1:tail, l2)
    }
}
fn returnConcat() -> fn([char], [char]) -> [char] {
    concat<char>
}

fn test() -> [char] {
    "Hallo" |> returnConcat()(identity("welt"))
})", samal::VMParameters{.functionsCallsPerGCRun = 0, .initialHeapSize = 0});
    auto vmRet = vm.run("Main.test", std::vector<samal::ExternalVMValue>{ });
    REQUIRE(vmRet.dump() == R"("Hallowelt")");
}
TEST_CASE("GC doesn't collect our parameters early for normal function calls", "[samal_whole_system]") {
    auto vm = compileSimple(R"(
fn identity<T>(p: T) -> T {
    p
}
fn concat<T>(l1 : [T], l2 : [T]) -> [T] {
    if l1 == [] {
        l2
    } else {
        l1:head + concat<T>(l1:tail, l2)
    }
}
fn returnConcat() -> fn([char], [char]) -> [char] {
    concat<char>
}

fn test() -> [char] {
    concat("Hallo", identity("welt"))
})", samal::VMParameters{.functionsCallsPerGCRun = 0, .initialHeapSize = 0});
    auto vmRet = vm.run("Main.test", std::vector<samal::ExternalVMValue>{ });
    REQUIRE(vmRet.dump() == R"("Hallowelt")");
}

TEST_CASE("GC doesn't collect our parameters early for tuples", "[samal_whole_system]") {
    auto vm = compileSimple(R"(
fn identity<T>(p: T) -> T {
    p
}

fn test() -> ([char], [char]) {
    ("Hallo", identity("Welt"))
})", samal::VMParameters{.functionsCallsPerGCRun = 0, .initialHeapSize = 0});
    auto vmRet = vm.run("Main.test", std::vector<samal::ExternalVMValue>{ });
    REQUIRE(vmRet.dump() == R"(("Hallo", "Welt"))");
}

TEST_CASE("GC doesn't collect our parameters early for lists", "[samal_whole_system]") {
    auto vm = compileSimple(R"(
fn identity<T>(p: T) -> T {
    p
}
fn test() -> [[char]] {
    ["Hallo", identity("Welt")]
})", samal::VMParameters{.functionsCallsPerGCRun = 0, .initialHeapSize = 0});
    auto vmRet = vm.run("Main.test", std::vector<samal::ExternalVMValue>{ });
    REQUIRE(vmRet.dump() == R"(["Hallo", "Welt"])");
}