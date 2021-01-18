#include "samal_lib/AST.hpp"
#include "samal_lib/Compiler.hpp"
#include "samal_lib/DatatypeCompleter.hpp"
#include "samal_lib/ExternalVMValue.hpp"
#include "samal_lib/Forward.hpp"
#include "samal_lib/Parser.hpp"
#include "samal_lib/VM.hpp"
#include <iostream>

int main() {
    samal::Stopwatch stopwatch{ "The main function" };
    samal::Parser parser;
    auto ast = parser.parse("Main", R"(
fn fib(n : i32) -> i32 {
    if n < 2 {
        n
    } else {
        fib(n-1)+fib(n-2)
    }
})");
    auto ast2 = parser.parse("Main2", R"(
fn fib(n : i64) -> i64 {
    if n < 2i64 {
        n
    } else {
        fib(n-1i64)+fib(n-2i64)
    }
}
fn func2(p : i32) -> i32 {
    x = add<i32>(2, 3)
    y = add<i64>(2i64, 3i64)
    z = add<i32>(2, 3)
    x
}
fn add<T>(a : T, b : T) -> T {
    a + b
})");
    assert(ast.first);
    assert(ast2.first);
    {
        samal::Stopwatch stopwatch2{ "Dumping the AST" };
        std::cout << ast.first->dump(0) << "\n";
        std::cout << ast2.first->dump(0) << "\n";
    }
    std::vector<samal::up<samal::ModuleRootNode>> modules;
    {
        samal::Stopwatch stopwatch2{ "Datatype completion + dump" };
        samal::DatatypeCompleter completer;
        modules.push_back(std::move(ast.first));
        modules.push_back(std::move(ast2.first));
        completer.declareModules(modules);
        completer.complete(modules.at(0));
        completer.complete(modules.at(1));
        std::cout << modules.at(0)->dump(0) << "\n";
        std::cout << modules.at(1)->dump(0) << "\n";
    }
    //auto cpy = modules;
    samal::Program program;
    {
        samal::Stopwatch stopwatch2{ "Compilation to bytecode + dump" };
        samal::Compiler comp;
        program = comp.compile(modules);
        std::cout << program.disassemble() << "\n";
    }
#if 0
    {
        samal::Stopwatch stopwatch2{ "Running fib(28) 5 times" };
        samal::VM vm{ std::move(program) };
        for(size_t i = 0; i < 5; ++i) {
            auto ret = vm.run("fib", { samal::ExternalVMValue::wrapInt64(10) });
            std::cout << "fib(28)=" << ret.dump() << "\n";
        }
    }
#endif
#if 1
    {
        samal::Stopwatch stopwatch2{ "func2(2)" };
        samal::VM vm{ std::move(program) };
        auto ret = vm.run("func2", { samal::ExternalVMValue::wrapInt64(2) });
        //auto ret = vm.run("func2", { 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 2, 0, 0, 0, 0, 0, 0, 0 });
        std::cout << "func2(2)=" << ret.dump() << "\n";
    }
#endif
}