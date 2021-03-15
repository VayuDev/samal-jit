#pragma once
#include "peg_parser/PegStructs.hpp"
#include <regex>
#include <stack>
#include <string>
#include <string_view>

namespace peg {

class PegTokenizer {
public:
    explicit PegTokenizer(std::string code);
    [[nodiscard]] const char* getPtr(ParsingState state) const;
    [[nodiscard]] ParsingState skipWhitespaces(ParsingState, bool newLines = true) const;
    [[nodiscard]] std::optional<ParsingState> matchString(ParsingState, const std::string_view& string) const;
    [[nodiscard]] std::optional<ParsingState> matchRegex(ParsingState, const std::regex& regex) const;
    [[nodiscard]] bool isEmpty(ParsingState) const;
    [[nodiscard]] std::pair<size_t, size_t> getPosition(ParsingState state) const;
    [[nodiscard]] size_t getRemainingBytesCount(ParsingState) const;
private:
    std::string mCode;
};

}
