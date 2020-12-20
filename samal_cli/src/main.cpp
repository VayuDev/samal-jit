#include <iostream>
#include "samal_lib/Parser.hpp"
#include "samal_lib/AST.hpp"
#include "samal_lib/DatatypeCompleter.hpp"
#include "samal_lib/Forward.hpp"

int main() {
  samal::Stopwatch stopwatch{"The main function"};
  samal::Parser parser;
  auto ast = parser.parse(R"(
fn fib(n : i32) -> i32 {
  x = add(3, 2);
  if n < 2 {
    n;
  } else {
    fib(n-1) + fib(n-2);
  };
}
fn add(a : i32, b : i32) -> i32 {
  a + b;
})");
  samal::Stopwatch stopwatch2{"Dumping the AST"};
  if(!ast)
    return 1;
  std::cout << ast->dump(0) << "\n";
  samal::DatatypeCompleter completer;
  std::vector<samal::up<samal::ModuleRootNode>> modules;
  modules.push_back(std::move(ast));
  completer.declareModules(modules);
  completer.complete(modules.at(0));
  std::cout << modules.at(0)->dump(0) << "\n";
}