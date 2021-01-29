#pragma once
#include "Datatype.hpp"
#include "Forward.hpp"
#include <cctype>
#include <map>
#include <string>
#include <vector>

namespace samal {

struct Program final {
    struct Function final {
        int32_t offset;
        int32_t len;
        Datatype type;
        std::string name;
        std::map<std::string, Datatype> templateParameters;
        sp<StackInformationTree> stackInformation;
        std::unordered_map<int32_t, int32_t> stackSizePerIp;
        ~Function();
    };
    std::vector<uint8_t> code;
    std::vector<Function> functions;
    [[nodiscard]] std::string disassemble() const;
};

}