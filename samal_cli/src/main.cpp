#include <iostream>
#include "samal_lib/Parser.hpp"
#include "samal_lib/AST.hpp"
#include "samal_lib/DatatypeCompleter.hpp"
#include "samal_lib/Forward.hpp"
#include "samal_lib/Compiler.hpp"
#include "samal_lib/VM.hpp"

int main() {
  samal::Stopwatch stopwatch{"The main function"};
  samal::Parser parser;
  auto ast = parser.parse("Main", R"(
fn fib(n : i32) -> i32 {
  if n < 2 {
    n
  } else {
    fib(n - 1) + fib(n - 2)
  }
})");
  auto ast2 = parser.parse("Magic", R"(
fn magicNumber(p : i32) -> i32 {
  x = p + 1
  i = if 10 < 5 {
    3
  } else {
    x = 3
    p + x
  }
  x + i + p
}
fn func2(p: i32) -> i32 {
  p + 1
})");
  assert(ast.first);
  assert(ast2.first);
  {
    samal::Stopwatch stopwatch2{"Dumping the AST"};
    std::cout << ast.first->dump(0) << "\n";
  }
  std::vector<samal::up<samal::ModuleRootNode>> modules;
  {
    samal::Stopwatch stopwatch2{"Datatype completion + dump"};
    samal::DatatypeCompleter completer;
    modules.push_back(std::move(ast.first));
    modules.push_back(std::move(ast2.first));
    completer.declareModules(modules);
    completer.complete(modules.at(0));
    completer.complete(modules.at(1));
    std::cout << modules.at(0)->dump(0) << "\n";
  }
  samal::Program program;
  {
    samal::Stopwatch stopwatch2{"Compilation to bytecode + dump"};
    samal::Compiler comp;
    program = comp.compile(modules);
    std::cout << program.disassemble() << "\n";
  }
#if 0
  {
    samal::Stopwatch stopwatch2{"Running fib(28) 5 times"};
    samal::VM vm{std::move(program)};
    for(size_t i = 0; i < 5; ++i) {
      auto ret = vm.run("fib", {28, 0, 0, 0});
      std::cout << "fib(28)=" << *(int32_t*)ret.data() << "\n";
    }
  }
#endif
  {
    samal::VM vm{std::move(program)};
    auto ret = vm.run("magicNumber", {5, 0, 0, 0, 0, 0, 0, 0});
  }
}