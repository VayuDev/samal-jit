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
    x = Templ.add<i32>(2, 7)
    x
})");
    auto ast2 = parser.parse("Templ", R"(
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
        modules.push_back(std::move(ast2.first));
        completer.declareModules(modules);
        // this loop should in theory be able to run asynchronously (each iteration in one thread)
        for(auto& module: modules) {
            auto completerCpy = completer;
            auto templatesUsedByThisModule = completerCpy.complete(module);
            // this would of course need to be synchronised
            templates.insert(std::move_iterator(templatesUsedByThisModule.begin()), std::move_iterator(templatesUsedByThisModule.end()));
        }
        std::cout << modules.at(0)->dump(0) << "\n";
        std::cout << modules.at(0)->dump(1) << "\n";
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