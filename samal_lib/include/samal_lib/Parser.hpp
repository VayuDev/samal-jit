#pragma once
#include "peg_parser/PegParser.hpp"
#include "samal_lib/Forward.hpp"
#include "samal_lib/Util.hpp"

namespace samal {

class Parser {
public:
    Parser();
    [[nodiscard]] std::pair<up<ModuleRootNode>, peg::PegTokenizer> parse(std::string moduleName, std::string code) const;

private:
    peg::PegParser mPegParser;
};

}