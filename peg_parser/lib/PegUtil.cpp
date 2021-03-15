#include "peg_parser/PegUtil.hpp"

namespace peg {

std::optional<UTF8Result> decodeUTF8Codepoint(std::string_view bytes) {
    size_t len = 0;
    int32_t value = 0;
    bool found = false;
    unsigned char firstChar = bytes.at(0);
    if(!(firstChar & 0b1000'0000)) {
        // single byte codepoint
        len = 1;
        value = firstChar;
        found = true;
    }
    if(bytes.length() <= 1)
        return {};
    unsigned char secondChar = bytes.at(1);
    if(!found && firstChar & 0b110'0000 && ~firstChar & 0b0010'0000) {
        // two byte codepoint
        len = 2;
        found = true;
        value = (firstChar & 0b0001'1111) | ((secondChar & 0b0011'1111) << 5);
    }
    if(bytes.length() <= 2)
        return {};
    unsigned char thirdChar = bytes.at(2);
    if(!found && firstChar & 0b111'0000 && ~firstChar & 0b0001'0000) {
        // three byte codepoint
        len = 3;
        found = true;
        value = ((firstChar & 0b0000'1111) << 12) | ((secondChar & 0b0011'1111) << 6) | ((thirdChar & 0b0011'1111) << 0);
    }
    if(bytes.length() <= 3)
        return {};
    unsigned char fourthChar = bytes.at(3);
    if(!found && firstChar & 0b111'1000 && ~firstChar & 0b0000'1000) {
        // four byte codepoint
        len = 4;
        found = true;
        value = ((firstChar & 0b0000'0111) << 18) | ((secondChar & 0b0011'1111) << 12) | ((thirdChar & 0b0011'1111) << 6) | ((fourthChar & 0b0011'1111) << 0);
    }
    if(value == 0 || !found) {
        return {};
    }
    return UTF8Result{value, len};
}

}