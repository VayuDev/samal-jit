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

fn zip<S, T>(l1 : [S], l2 : [T]) -> [(S, T)] {
    if l1 == [] || l2 == [] {
        [:(S, T)]
    } else {
        (l1:head, l2:head) + zip<S, T>(l1:tail, l2:tail)
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

fn testListSpeed(p : i32) -> [i32] {
    l1 = seq<i32>(5000)
    l2 = seq<i32>(5000)
    combined = concat<i32>(l1, l2)
    lambda = fn(p2 : i32) -> i32 {
        p2
    }
    added = map<i32, i32>(combined, lambda)
}

fn testZip(p : i32) -> [(i32, i32)] {
    l1 = seq<i32>(20000)
    l2 = seq<i32>(20000)
    l1Added = map<i32, i32>(l1, fn(p : i32) -> i32 {
        p / 16
    })
    zip<i32, i32>(l1Added, l2)
}

struct Vec2 {
    x : i32,
    y : i32
}

struct Rect {
    pos : Vec2,
    size : Vec2
}

fn testStruct(p : i32) -> Vec2 {

}

fn sum<T>(list : [T]) -> T {
    if list == [] {
        0
    } else {
        list:head + sum<T>(list:tail)
    }
}

fn avg<T>(list : [T]) -> T {
    s = sum<T>(list)
    l = len<T>(list)
    s / l
}

fn testAvg(p : i32) -> i32 {
    avg<i32>([1, 3, 5, 7])
}

fn testFib(p : i32) -> i32 {
    Templ.fib<i32>(28)
    Templ.fib<i32>(28)
    Templ.fib<i32>(28)
    Templ.fib<i32>(28)
    Templ.fib<i32>(28)
}
fn testSimple(p : i32) -> [i32] {

   seq<i32>(20)
    |> map<i32, i32>(fn(p : i32) -> i32 {
        ((p - 100) * 2) / -2
    })
}

fn makeLambda<T>(v : T) -> fn(T) -> T {
    print<T>(v)
    fn(p2: T) -> T {
        v + p2
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
        samal::Datatype{ samal::Datatype::createEmptyTuple(), { samal::Datatype("T") } },
        [](samal::VM& vm, const std::vector<samal::ExternalVMValue>& params) -> samal::ExternalVMValue {
            std::cout << "[Code] " << params.at(0).dump() << "\n";
            return samal::ExternalVMValue::wrapEmptyTuple(vm);
        } });
    nativeFunctions.emplace_back(samal::NativeFunction{
        "Main.print",
        samal::Datatype{ samal::Datatype::createEmptyTuple(), { samal::Datatype{ samal::DatatypeCategory::i32 } } },
        [](samal::VM& vm, const std::vector<samal::ExternalVMValue>& params) -> samal::ExternalVMValue {
            printf("[Code i32] %i\n", params.at(0).as<int32_t>());
            return samal::ExternalVMValue::wrapEmptyTuple(vm);
        } });
    nativeFunctions.emplace_back(samal::NativeFunction{
        "Main.print",
        samal::Datatype{ samal::Datatype::createEmptyTuple(), { samal::Datatype{ samal::DatatypeCategory::i64 } } },
        [](samal::VM& vm, const std::vector<samal::ExternalVMValue>& params) -> samal::ExternalVMValue {
            printf("[Code i64] %li\n", params.at(0).as<int64_t>());
            return samal::ExternalVMValue::wrapEmptyTuple(vm);
        } });
    nativeFunctions.emplace_back(samal::NativeFunction{
        "Main.identity",
        samal::Datatype{ samal::Datatype("T"), { samal::Datatype("T") } },
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
    auto ret = vm.run("Main.testAvg", std::vector<samal::ExternalVMValue>{ samal::ExternalVMValue::wrapInt32(vm, 5) });
    std::cout << "Func2 returned " << ret.dump() << "\n";
    vmStopwatch.stop();
}