#pragma once
#include "Datatype.hpp"
#include <cctype>
#include <map>
#include <string>
#include <vector>

namespace samal {

struct Program {
    struct Function {
        int32_t offset;
        int32_t len;
        Datatype type;
    };
    std::vector<uint8_t> code;
    std::map<std::string, Function> functions;
    [[nodiscard]] std::string disassemble() const;
};

}