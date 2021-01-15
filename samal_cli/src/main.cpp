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
}
fn join(a : i32, b : i32) -> (i32, i32) {
    (a, b)
}
fn add(a : i32, b : i32) -> i32 {
    a + b
}
fn sub(a : i32, b : i32) -> i32 {
    a - b
}
fn func2(p : i64) -> i64 {
    t = (p, 1)
    if false {
        t:0
    } else {
        42i64
    }
})");
    assert(ast.first);
    {
        samal::Stopwatch stopwatch2{ "Dumping the AST" };
        std::cout << ast.first->dump(0) << "\n";
    }
    std::vector<samal::up<samal::ModuleRootNode>> modules;
    {
        samal::Stopwatch stopwatch2{ "Datatype completion + dump" };
        samal::DatatypeCompleter completer;
        modules.push_back(std::move(ast.first));
        completer.declareModules(modules);
        completer.complete(modules.at(0));
        std::cout << modules.at(0)->dump(0) << "\n";
    }
    samal::Program program;
    {
        samal::Stopwatch stopwatch2{ "Compilation to bytecode + dump" };
        samal::Compiler comp;
        program = comp.compile(modules);
        std::cout << program.disassemble() << "\n";
    }
#if 1
    {
        samal::Stopwatch stopwatch2{ "Running fib(28) 5 times" };
        samal::VM vm{ std::move(program) };
        for(size_t i = 0; i < 5; ++i) {
            auto ret = vm.run("fib", { samal::ExternalVMValue::wrapInt32(28) });
            std::cout << "fib(28)=" << ret.dump() << "\n";
        }
    }
#endif
#if 0
    {
        samal::Stopwatch stopwatch2{ "func2(2)" };
        samal::VM vm{ std::move(program) };
        auto ret = vm.run("func2", { samal::ExternalVMValue::wrapInt64(2) });
        //auto ret = vm.run("func2", { 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 2, 0, 0, 0, 0, 0, 0, 0 });
        std::cout << "func2(2)=" << ret.dump() << "\n";
    }
#endif
}