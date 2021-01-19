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
fn func2(p : i32) -> i32 {
    b = add<i64>(2i64, 7i64)
    add<i32>(2, 7)
}
fn add<T>(a : T, b : T) -> T {
    a + b
})");
    assert(ast.first);
    {
        samal::Stopwatch stopwatch2{ "Dumping the AST" };
        std::cout << ast.first->dump(0) << "\n";
    }
    std::vector<samal::up<samal::ModuleRootNode>> modules;
    std::map<const samal::IdentifierNode*, samal::TemplateInstantiationInfo> templates;
    {
        samal::Stopwatch stopwatch2{ "Datatype completion + dump" };
        samal::DatatypeCompleter completer;
        modules.push_back(std::move(ast.first));
        completer.declareModules(modules);
        templates = completer.complete(modules.at(0));
        std::cout << modules.at(0)->dump(0) << "\n";
    }
    //auto cpy = modules;
    samal::Program program;
    {
        samal::Stopwatch stopwatch2{ "Compilation to bytecode + dump" };
        samal::Compiler comp;
        program = comp.compile(modules, templates);
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
        auto ret = vm.run("func2", { samal::ExternalVMValue::wrapInt32(2) });
        //auto ret = vm.run("func2", { 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 2, 0, 0, 0, 0, 0, 0, 0 });
        std::cout << "func2(2)=" << ret.dump() << "\n";
    }
#endif
}