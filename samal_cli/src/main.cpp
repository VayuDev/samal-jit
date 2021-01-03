#include <iostream>
#include "samal_lib/Parser.hpp"
#include "samal_lib/AST.hpp"
#include "samal_lib/DatatypeCompleter.hpp"
#include "samal_lib/Forward.hpp"
#include "samal_lib/Compiler.hpp"

int main() {
  samal::Stopwatch stopwatch{"The main function"};
  samal::Parser parser;
  auto ast = parser.parse("Main", R"(
fn fib(n : i32) -> i32 {
  a = 77
  if n < 2 {
    n
  } else {
    fib(n-1) + fib(n-2)
  }
})");
  auto ast2 = parser.parse("Magic", R"(
fn magicNumber() -> i32 {
  5
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
  {
    samal::Compiler comp;
    auto program = comp.compile(modules);
    std::cout << program.disassemble() << "\n";
  }
}