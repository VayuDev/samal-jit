#pragma once

#include <memory>
#include <optional>
#include <string_view>

namespace peg {

template<typename T>
using sp = std::shared_ptr<T>;

template<typename T>
using up = std::unique_ptr<T>;

template<typename T>
using wp = std::weak_ptr<T>;

struct UTF8Result {
    int32_t utf32Value;
    size_t len;
};

std::optional<UTF8Result> decodeUTF8Codepoint(std::string_view bytes);

}
