#include <iostream>
#include "samal_lib/Parser.hpp"
#include "samal_lib/AST.hpp"

int main() {
  samal::Parser parser;
  auto ast = parser.parse(R"(fn main(x : i32, y : i32) -> i32 { 0 })");
}