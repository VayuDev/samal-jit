#pragma once
#include <vector>
#include <cctype>
#include <map>
#include <string>

namespace samal {

struct Program {
  struct Function {
    int32_t offset;
    int32_t len;
  };
  std::vector<uint8_t> code;
  std::map<std::string, Function> functions;
  [[nodiscard]] std::string disassemble() const;
};

}