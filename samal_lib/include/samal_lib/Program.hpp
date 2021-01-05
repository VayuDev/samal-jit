#pragma once
#include <vector>
#include <cctype>
#include <string>

namespace samal {

struct Program {
  struct Function {
    std::vector<uint8_t> code;
    std::string name;
  };
  std::vector<Function> functions;
  [[nodiscard]] std::string disassemble() const;
};

}