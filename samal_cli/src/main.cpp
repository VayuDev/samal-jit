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
native fn print<T>(p: T) -> ()
native fn identity<T>(p: T) -> T

fn len<T>(l : [T]) -> i32 {
    if l == [:T] {
        0
    } else {
        1 + len<T>(l:tail)
    }
}

fn concat<T>(l1 : [T], l2 : [T]) -> [T] {
    if l1 == [:T] {
        l2
    } else {
        l1:head + concat<T>(l1:tail, l2)
    }
}

fn map<T, S>(l : [T], callback : fn(T) -> S) -> [S] {
    if l == [:T] {
        [:S]
    } else {
        callback(l:head) + map<T, S>(l:tail, callback)
    }
}

fn createList<T>(limit: T) -> [T] {
    if limit < 1 {
        [:T]
    } else {
        limit + createList<T>(limit - 1)
    }
}

fn func2(p : i32) -> [i32] {
    l1 = createList<i32>(1000)
    l2 = createList<i32>(1000)
    combined = concat<i32>(l1, l2)
    lambda = fn(p2 : i32) -> i32 {
        p2 + 1
    }
    added = map<i32, i32>(combined, lambda)
}

fn makeLambda<T>(v : T) -> fn(T) -> T {
    print<T>(v)
    fn(p2: T) -> T {
        v + p2
    }
})");
    auto ast2 = parser.parse("Templ", R"(
fn fib<T>(a : T) -> T {
    Main.print<T>(a)
    if a < 2 {
        Main.identity<T>(a)
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
        samal::Datatype{samal::Datatype::createEmptyTuple(), { samal::Datatype("T")}},
        [](samal::VM& vm, const std::vector<samal::ExternalVMValue>& params) -> samal::ExternalVMValue {
            std::cout << "[Code] " << params.at(0).dump() << "\n";
            return samal::ExternalVMValue::wrapEmptyTuple(vm);
        } });
    nativeFunctions.emplace_back(samal::NativeFunction{
        "Main.print",
        samal::Datatype{samal::Datatype::createEmptyTuple(), { samal::Datatype{ samal::DatatypeCategory::i32 } }},
        [](samal::VM& vm, const std::vector<samal::ExternalVMValue>& params) -> samal::ExternalVMValue {
            printf("[Code i32] %i\n", params.at(0).as<int32_t>());
            return samal::ExternalVMValue::wrapEmptyTuple(vm);
        } });
    nativeFunctions.emplace_back(samal::NativeFunction{
        "Main.print",
        samal::Datatype{samal::Datatype::createEmptyTuple(), { samal::Datatype{ samal::DatatypeCategory::i64 } }},
        [](samal::VM& vm, const std::vector<samal::ExternalVMValue>& params) -> samal::ExternalVMValue {
            printf("[Code i64] %li\n", params.at(0).as<int64_t>());
            return samal::ExternalVMValue::wrapEmptyTuple(vm);
        } });
    nativeFunctions.emplace_back(samal::NativeFunction{
        "Main.identity",
        samal::Datatype{samal::Datatype("T"), { samal::Datatype("T") }},
        [](samal::VM& vm, const std::vector<samal::ExternalVMValue>& params) -> samal::ExternalVMValue {
            std::cout << "Identity function called on " << params.at(0).dump() << "\n";
            return params.at(0);
        } });
    samal::Compiler compiler{ modules, std::move(nativeFunctions) };
    auto program = compiler.compile();
    std::cout << "Code dump:\n" + program.disassemble();
    compilerStopwatch.stop();

    samal::Stopwatch vmStopwatch{ "VM execution" };
    samal::VM vm{ std::move(program) };
    auto ret = vm.run("Main.func2", std::vector<samal::ExternalVMValue>{ samal::ExternalVMValue::wrapInt32(vm, 5) });
    std::cout << "Func2 returned " << ret.dump() << "\n";
    vmStopwatch.stop();
}