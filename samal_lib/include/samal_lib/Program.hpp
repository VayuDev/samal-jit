#pragma once
#include "Datatype.hpp"
#include "Forward.hpp"
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
        std::string name;
        std::map<std::string, Datatype> templateParameters;
    };
    std::vector<uint8_t> code;
    std::vector<Function> functions;
    [[nodiscard]] std::string disassemble() const;
};

}