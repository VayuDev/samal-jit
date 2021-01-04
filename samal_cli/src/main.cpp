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
fn main() -> i32 {
  i = Magic.magicNumber
  x = i(5)
  j = i
  5 + x
})");
  auto ast2 = parser.parse("Magic", R"(
fn magicNumber(p : i32) -> i32 {
  42
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
    samal::Stopwatch stopwatch2{"Compilation to bytecode + dump"};
    samal::Compiler comp;
    auto program = comp.compile(modules);
    std::cout << program.disassemble() << "\n";
  }
}