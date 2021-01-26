#include "samal_lib/AST.hpp"
#include "samal_lib/Compiler.hpp"
#include "samal_lib/ExternalVMValue.hpp"
#include "samal_lib/Forward.hpp"
#include "samal_lib/Parser.hpp"
#include "samal_lib/VM.hpp"
#include <iostream>

int main() {
    samal::Stopwatch stopwatch{ "The main function" };
    samal::Parser parser;
    samal::Stopwatch parserStopwatch{ "Parsing" };
    auto ast = parser.parse("Main", R"(
fn func2(p : i32) -> i64 {
    lambda = makeLambda<i32>(5)
    lambda2 = makeLambda<i64>(24i64)
    lambda2(10i64)
}

fn makeLambda<T>(p : T) -> fn(T) -> T {
    fn(p2: T) -> T {
        p + p2
    }
})");
    auto ast2 = parser.parse("Templ", R"(
fn fib<T>(a : T) -> T {
    if a < 2 {
        a
    } else {
        fib<T>(a - 1) + fib<T>(a - 2)
    }
}
fn add<T>(a : T, b : T) -> T {
    a + b
}
fn addAndFib<T>(a : T, b : T) -> T {
    fib<T>(a) + b
})");
    parserStopwatch.stop();
    assert(ast.first);
    {
        samal::Stopwatch stopwatch2{ "Dumping the AST" };
        std::cout << ast.first->dump(0) << "\n";
        std::cout << ast2.first->dump(0) << "\n";
    }
    samal::Stopwatch compilerStopwatch{ "Compiling the code + dump" };
    std::vector<samal::up<samal::ModuleRootNode>> modules;
    modules.emplace_back(std::move(ast.first));
    modules.emplace_back(std::move(ast2.first));
    samal::Compiler compiler{ modules };
    auto program = compiler.compile();
    std::cout << "Code dump:\n" + program.disassemble();
    compilerStopwatch.stop();

    samal::Stopwatch vmStopwatch{ "VM execution" };
    samal::VM vm{ std::move(program) };
    auto ret = vm.run("Main.func2", { samal::ExternalVMValue::wrapInt32(1) });
    std::cout << "Func2 returned " << ret.dump() << "\n";
    vmStopwatch.stop();
}