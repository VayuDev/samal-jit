#include <iostream>
#include "samal_lib/Parser.hpp"
#include "samal_lib/AST.hpp"
#include "samal_lib/DatatypeCompleter.hpp"
#include "samal_lib/Forward.hpp"

int main() {
  samal::Stopwatch stopwatch{"The main function"};
  samal::Parser parser;
  auto ast = parser.parse("Main", R"(
fn fib(n : i32) -> i32 {
  f = Magic.magicNumber()
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
  {
    samal::Stopwatch stopwatch2{"Datatype completion + dump"};
    samal::DatatypeCompleter completer;
    std::vector<samal::up<samal::ModuleRootNode>> modules;
    modules.push_back(std::move(ast.first));
    modules.push_back(std::move(ast2.first));
    completer.declareModules(modules);
    completer.complete(modules.at(0));
    std::cout << modules.at(0)->dump(0) << "\n";
  }
}