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
native fn print(p: i32) -> ()

fn func2(p : i32) -> i32 {
    l = makeLambda<i32>(5)
    x = l(1)
    print(x)
    x
}

fn makeLambda<T>(v : T) -> fn(T) -> T {
    fn(p2: T) -> T {
        v + p2
    }
})");
    auto ast2 = parser.parse("Templ", R"(
fn fib<T>(a : T) -> T {
    Main.print(a)
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
    assert(ast2.first);
    {
        samal::Stopwatch stopwatch2{ "Dumping the AST" };
        std::cout << ast.first->dump(0) << "\n";
        std::cout << ast2.first->dump(0) << "\n";
    }
    samal::Stopwatch compilerStopwatch{ "Compiling the code + dump" };
    std::vector<samal::up<samal::ModuleRootNode>> modules;
    modules.emplace_back(std::move(ast.first));
    modules.emplace_back(std::move(ast2.first));
    std::vector<samal::NativeFunction> nativeFunctions;
    nativeFunctions.emplace_back(samal::NativeFunction{
        "Main.print",
        samal::Datatype::createEmptyTuple(),
        { samal::Datatype{ samal::DatatypeCategory::i32 } },
        [](const std::vector<samal::ExternalVMValue>& params) -> samal::ExternalVMValue {
            printf("[Code] %i\n", params.at(0).as<int32_t>());
            return samal::ExternalVMValue::wrapEmptyTuple();
        } });
    samal::Compiler compiler{ modules, std::move(nativeFunctions) };
    auto program = compiler.compile();
    std::cout << "Code dump:\n" + program.disassemble();
    compilerStopwatch.stop();

    samal::Stopwatch vmStopwatch{ "VM execution" };
    samal::VM vm{ std::move(program) };
    auto ret = vm.run("Main.func2", { samal::ExternalVMValue::wrapInt32(1) });
    std::cout << "Func2 returned " << ret.dump() << "\n";
    vmStopwatch.stop();
}