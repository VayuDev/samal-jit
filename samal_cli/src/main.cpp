#include <iostream>
#include "samal_lib/Parser.hpp"
#include "samal_lib/AST.hpp"

int main() {
  samal::Stopwatch stopwatch{"The main function"};
  samal::Parser parser;
  auto ast = parser.parse(R"(
fn main(x5 : List3, y : i32) -> i32 {
  5 + {3 - 2;} * 4;
}
fn fib(n : i32) -> i32 {

})");
  samal::Stopwatch stopwatch2{"Dumping the AST"};
  if(ast)
    std::cout << ast->dump(0) << "\n";
}