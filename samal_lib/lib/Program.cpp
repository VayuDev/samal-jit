#include "samal_lib/Program.hpp"
#include "samal_lib/Instruction.hpp"
#include "samal_lib/StackInformationTree.hpp"

namespace samal {

Program::Function::~Function() = default;

std::string Program::disassemble() const {
    std::string ret;
    for(auto& func : functions) {
        ret += "Function " + func.name;
        if(!func.templateParameters.empty()) {
            ret += "<";
            size_t i = 0;
            for(auto& param : func.templateParameters) {
                ret += param.first + " => " + param.second.toString();
                if(++i < func.templateParameters.size()) {
                    ret += ",";
                }
            }
            ret += ">";
        }
        ret += "\n";
        int32_t offset = func.offset;
        while(offset < func.offset + func.len) {
            ret += " " + std::to_string(offset) + " ";
            ret += instructionToString(static_cast<Instruction>(code.at(offset)));
            auto width = instructionToWidth(static_cast<Instruction>(code.at(offset)));
            if(width >= 5) {
                ret += " ";
                ret += std::to_string(*reinterpret_cast<const int32_t*>(&code.at(offset + 1)));
            }
            if(width >= 9) {
                ret += " ";
                ret += std::to_string(*reinterpret_cast<const int32_t*>(&code.at(offset + 5)));
            }
            ret += "\n";
            offset += width;
        }
        ret += "\n";
    }
    return ret;
}
}