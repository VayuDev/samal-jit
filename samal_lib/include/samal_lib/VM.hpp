#pragma once

#include "Program.hpp"
#include <xbyak/xbyak.h>

namespace samal {

class Stack final {
 public:
  Stack();
  ~Stack();
  void push(const std::vector<uint8_t>&);
  void push(const void* data, size_t len);
  void repush(size_t offset, size_t len);
  void popBelow(size_t offset, size_t len);
  void pop(size_t len);
  void* get(size_t offset);
  std::string dump();
  std::vector<uint8_t> moveData();
  uint8_t* getBasePtr();
  size_t getSize();
  void setSize(size_t);
  void clear();
 private:
  void ensureSpace(size_t additionalLen);

  uint8_t *mData;
  size_t mDataLen { 0 };
  size_t mDataReserved { 0 };
};

class VM final {
 public:
  explicit VM(Program program);
  std::vector<uint8_t> run(const std::string& functionName, const std::vector<uint8_t>& initialStack);
 private:
  __always_inline bool interpretInstruction();
  __always_inline bool jitCompileAndRunInstruction();

  Stack mStack;
  Program mProgram;
  uint32_t mIp;
  struct ProgramSegment {
    int32_t startIp;
    int32_t len;
    std::function<bool()> callback;
  };
  size_t mMainFunctionReturnTypeSize = 0;
  std::vector<ProgramSegment> mSegments;
};

}