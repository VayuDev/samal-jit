#pragma once
#include <cstddef>

namespace peg {

struct ParsingState {
  size_t tokenizerState = 0;
};

}