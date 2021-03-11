#pragma once
#include "Datatype.hpp"
#include "Forward.hpp"
#include <cctype>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

namespace samal {

struct NativeFunction {
    std::string fullName;
    Datatype functionType;
    mutable std::function<ExternalVMValue(VM&, const std::vector<ExternalVMValue>&)> callback;
};

struct Program final {
    struct Function final {
        int32_t offset;
        int32_t len;
        Datatype type;
        std::string name;
        UndeterminedIdentifierReplacementMap templateParameters;
        sp<StackInformationTree> stackInformation;
        std::unordered_map<int32_t, int32_t> stackSizePerIp;
        ~Function();
    };
    std::vector<uint8_t> code;
    std::vector<Function> functions;
    std::vector<Datatype> auxiliaryDatatypes;
    std::vector<NativeFunction> nativeFunctions;
    [[nodiscard]] std::string disassemble() const;
};

}