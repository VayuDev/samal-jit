#pragma once

#include "Program.hpp"

namespace samal {

class Stack {
 public:
  void push(const std::vector<uint8_t>&);
  void push(const void* data, size_t len);
  void repush(size_t offset, size_t len);
  void popBelow(size_t offset, size_t len);
  void pop(size_t len);
  void* get(size_t offset);
  std::string dump();
  std::vector<uint8_t> moveData();
 private:
  std::vector<uint8_t> mData;
};

class VM final {
 public:
  explicit VM(Program program);
  std::vector<uint8_t> run(const std::string& function, const std::vector<uint8_t>& initialStack);
 private:
  int32_t mCurrentFunctionId { -1 };
  Program::Function* mCurrentFunction { nullptr };
  Stack mStack;
  Program mProgram;
  uint32_t mIp;
};

}