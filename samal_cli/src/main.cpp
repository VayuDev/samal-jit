#include <iostream>
#include "samal_lib/Parser.hpp"
#include "samal_lib/AST.hpp"

int main() {
  samal::Stopwatch stopwatch{"The main function"};
  samal::Parser parser;
  auto ast = parser.parse(R"(
fn fib(n : i32) -> i32 {
  x = n;
  if n < 2 {
    n;
  } else {
    fib(n-1) + fib(n-2);
  };
})");
  samal::Stopwatch stopwatch2{"Dumping the AST"};
  if(ast)
    std::cout << ast->dump(0) << "\n";
}