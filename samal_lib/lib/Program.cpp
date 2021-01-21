#include "samal_lib/Program.hpp"
#include "samal_lib/Instruction.hpp"

namespace samal {

std::string Program::disassemble() const {
    std::string ret;
    for(auto& [name, location] : functions) {
        ret += "Function " + name + ":\n";
        size_t offset = location.offset;
        while(offset < location.offset + location.len) {
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