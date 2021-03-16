#include "peg_parser/PegUtil.hpp"
#include <codecvt>
#include <locale>

namespace peg {

std::optional<UTF8Result> decodeUTF8Codepoint(std::string_view bytes) {
    int32_t value = 0;
    unsigned char firstChar = bytes.at(0);
    if(!(firstChar & 0b1000'0000)) {
        // single byte codepoint
        value = firstChar;
        return UTF8Result{value, 1};
    }
    if(bytes.length() <= 1)
        return {};
    unsigned char secondChar = bytes.at(1);
    if(firstChar & 0b110'0000 && ~firstChar & 0b0010'0000) {
        // two byte codepoint
        value = ((firstChar & 0b0001'1111) << 6) | ((secondChar & 0b0011'1111) << 0);
        return UTF8Result{value, 2};
    }
    if(bytes.length() <= 2)
        return {};
    unsigned char thirdChar = bytes.at(2);
    if(firstChar & 0b111'0000 && ~firstChar & 0b0001'0000) {
        // three byte codepoint
        value = ((firstChar & 0b0000'1111) << 12) | ((secondChar & 0b0011'1111) << 6) | ((thirdChar & 0b0011'1111) << 0);
        return UTF8Result{value, 3};
    }
    if(bytes.length() <= 3)
        return {};
    unsigned char fourthChar = bytes.at(3);
    if(firstChar & 0b111'1000 && ~firstChar & 0b0000'1000) {
        // four byte codepoint
        value = ((firstChar & 0b0000'0111) << 18) | ((secondChar & 0b0011'1111) << 12) | ((thirdChar & 0b0011'1111) << 6) | ((fourthChar & 0b0011'1111) << 0);
        return UTF8Result{value, 4};
    }
    return {};
}
[[maybe_unused]] std::string encodeUTF8Codepoint(int32_t codepoint) {
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter{"<invalid utf-32>"};
    return converter.to_bytes(std::u32string{static_cast<char32_t>(codepoint)});
}

}