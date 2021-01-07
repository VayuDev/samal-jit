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
 private:
  void ensureSpace(size_t additionalLen);

  uint8_t *mData;
  size_t mDataLen { 0 };
  size_t mDataReserved { 0 };
};

class VM final {
 public:
  explicit VM(Program program);
  std::vector<uint8_t> run(const std::string& function, const std::vector<uint8_t>& initialStack);
 private:
  __always_inline bool interpretInstruction();
  __always_inline bool jitCompileAndRunInstruction();

  int32_t mCurrentFunctionId { -1 };
  Program::Function* mCurrentFunction { nullptr };
  Stack mStack;
  Program mProgram;
  uint32_t mIp;
  int mFunctionDepth { 0 };
};

}