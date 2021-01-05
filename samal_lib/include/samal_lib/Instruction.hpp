#pragma once
#include <cstdint>
#include <cassert>
#include <cstddef>

namespace samal {

#define ENUMERATE_INSTRUCTIONS                \
  INSTRUCTION(INVALID, 5)                     \
  INSTRUCTION(PUSH_4, 5)                      \
  INSTRUCTION(PUSH_8, 9)                      \
  INSTRUCTION(POP_N_BELOW, 9)                 \
  INSTRUCTION(ADD_I32, 1)                     \
  INSTRUCTION(SUB_I32, 1)                     \
  INSTRUCTION(COMPARE_LESS_THAN_I32, 1)       \
  INSTRUCTION(COMPARE_MORE_THAN_I32, 1)       \
  INSTRUCTION(COMPARE_LESS_EQUAL_THAN_I32, 1) \
  INSTRUCTION(COMPARE_MORE_EQUAL_THAN_I32, 1) \
  INSTRUCTION(JUMP_IF_FALSE, 5)           \
  INSTRUCTION(JUMP, 5)                        \
  INSTRUCTION(REPUSH_N, 5)                    \
  INSTRUCTION(REPUSH_FROM_N, 9)               \
  INSTRUCTION(RETURN, 5)                      \
  INSTRUCTION(CALL, 5)

enum class Instruction : uint8_t {
#define INSTRUCTION(name, width) name,
  ENUMERATE_INSTRUCTIONS
#undef INSTRUCTION
};

static inline constexpr const char* instructionToString(Instruction ins) {
  constexpr const char* instructionNames[] = {
#define INSTRUCTION(name, width) #name,
      ENUMERATE_INSTRUCTIONS
#undef INSTRUCTION
  };
  const auto instructionIndex = static_cast<uint8_t>(ins);
  assert(instructionIndex >= 0 && instructionIndex < (sizeof(instructionNames) / sizeof(char*)));
  return instructionNames[instructionIndex];
}

static constexpr inline size_t instructionToWidth(Instruction ins) {
  constexpr size_t instructionWidths[] = {
#define INSTRUCTION(name, width) width,
      ENUMERATE_INSTRUCTIONS
#undef INSTRUCTION
  };
  const auto instructionIndex = static_cast<uint8_t>(ins);
  assert(instructionIndex >= 0 && instructionIndex < (sizeof(instructionWidths) / sizeof(char*)));
  return instructionWidths[instructionIndex];
}

}