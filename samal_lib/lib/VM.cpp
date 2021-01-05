#include <cassert>
#include "samal_lib/VM.hpp"
#include "samal_lib/Instruction.hpp"
#include <cstring>
#include <cstdlib>

namespace samal {

VM::VM(samal::Program program)
    : mProgram(std::move(program)) {

}
std::vector<uint8_t> VM::run(const std::string &function, const std::vector<uint8_t> &initialStack) {
  mStack.push(initialStack);
  int32_t id = 0;
  for(auto& func: mProgram.functions) {
    if(func.name == function) {
      mCurrentFunction = &func;
      mCurrentFunctionId = id;
      break;
    }
    ++id;
  }
  assert(mCurrentFunction);
  mIp = 0;
  int functionDepth = 1;
  while(functionDepth > 0) {
    auto ins = static_cast<Instruction>(mCurrentFunction->code.at(mIp));
    //printf("Executing instruction %i: %s\n", static_cast<int>(ins), instructionToString(ins));
    bool incIp = true;
    switch(ins) {
      case Instruction::PUSH_4:
        mStack.push(&mCurrentFunction->code.at(mIp + 1), 4);
        break;
      case Instruction::PUSH_8:
        mStack.push(&mCurrentFunction->code.at(mIp + 1), 8);
        break;
      case Instruction::REPUSH_FROM_N:
        mStack.repush(*(int32_t*)&mCurrentFunction->code.at(mIp + 5), *(int32_t*)&mCurrentFunction->code.at(mIp + 1));
        break;
      case Instruction::REPUSH_N:
        mStack.repush(0, *(int32_t*)&mCurrentFunction->code.at(mIp + 1));
        break;
      case Instruction::COMPARE_LESS_THAN_I32: {
        auto lhs = *(int32_t*)mStack.get(8);
        auto rhs = *(int32_t*)mStack.get(4);
        mStack.pop(8);
        bool res = lhs < rhs;
        mStack.push(&res, 1);
        break;
      }
      case Instruction::JUMP_IF_FALSE: {
        auto val = *(bool*)mStack.get(1);
        mStack.pop(1);
        if(!val) {
          mIp = *(int32_t*)&mCurrentFunction->code.at(mIp + 1);
          incIp = false;
        }
        break;
      }
      case Instruction::JUMP: {
        mIp = *(int32_t*)&mCurrentFunction->code.at(mIp + 1);
        incIp = false;
        break;
      }
      case Instruction::SUB_I32: {
        auto lhs = *(int32_t*)mStack.get(8);
        auto rhs = *(int32_t*)mStack.get(4);
        int32_t res = lhs - rhs;
        mStack.pop(8);
        mStack.push(&res, 4);
        break;
      }
      case Instruction::ADD_I32: {
        auto lhs = *(int32_t*)mStack.get(8);
        auto rhs = *(int32_t*)mStack.get(4);
        int32_t res = lhs + rhs;
        mStack.pop(8);
        mStack.push(&res, 4);
        break;
      }
      case Instruction::POP_N_BELOW: {
        mStack.popBelow(*(int32_t *) &mCurrentFunction->code.at(mIp + 5),
                        *(int32_t *) &mCurrentFunction->code.at(mIp + 1));
        break;
      }
      case Instruction::CALL: {
        auto offset = *(int32_t*)&mCurrentFunction->code.at(mIp + 1);
        auto functionId = *(int32_t*)mStack.get(offset + 8);
        // save old values to stack
        *(int32_t*)mStack.get(offset + 8) = mIp + instructionToWidth(Instruction::CALL);
        *(int32_t*)mStack.get(offset + 4) = mCurrentFunctionId;
        mCurrentFunction = &mProgram.functions.at(functionId);
        mCurrentFunctionId = functionId;
        mIp = 0;
        functionDepth += 1;
        incIp = false;
        break;
      }
      case Instruction::RETURN: {
        if(functionDepth == 1) {
          return mStack.moveData();
        }
        functionDepth -= 1;
        auto offset = *(int32_t*)&mCurrentFunction->code.at(mIp + 1);
        mIp = *(int32_t*)mStack.get(offset + 8);
        mCurrentFunctionId = *(int32_t*)mStack.get(offset + 4);
        mCurrentFunction = &mProgram.functions.at(mCurrentFunctionId);
        mStack.popBelow(offset, 8);
        incIp = false;
        break;
      }
      default:
        fprintf(stderr, "Unhandled instruction %i: %s\n", static_cast<int>(ins), instructionToString(ins));
        assert(false);
    }
    mIp += instructionToWidth(ins) * incIp;
    //auto dump = mStack.dump();
    //printf("Dump:\n%s\n", dump.c_str());
  }
  assert(false);
}
void Stack::push(const std::vector<uint8_t> &data) {
  push(data.data(), data.size());
}
void Stack::push(const void *data, size_t len) {
  ensureSpace(len);
  memcpy(mData + mDataLen, data, len);
  mDataLen += len;
}
void Stack::repush(size_t offset, size_t len) {
  assert(mDataLen >= offset + len);
  ensureSpace(len);
  memcpy(mData + mDataLen, mData + mDataLen - offset - len, len);
  mDataLen += len;
}
void *Stack::get(size_t offset) {
  return mData + mDataLen - offset;
}
void Stack::popBelow(size_t offset, size_t len) {
  assert(mDataLen >= offset + len);
  memmove(mData + mDataLen - offset - len, mData + mDataLen - offset, offset);
  mDataLen -= len;
}
void Stack::pop(size_t len) {
  mDataLen -= len;
}
std::string Stack::dump() {
  std::string ret;
  for(size_t i = 0; i < mDataLen; ++i) {
    uint8_t val = mData[i];
    ret += std::to_string(val) + " ";
    ++i;
    if(i > 0 && i % 4 == 0) {
      ret += "\n";
    }
  }
  return ret;
}
std::vector<uint8_t> Stack::moveData() {
  std::vector<uint8_t> ret;
  ret.resize(mDataLen);
  memcpy(ret.data(), mData, mDataLen);
  return ret;
}
Stack::Stack() {
  mDataReserved = 1024 * 10;
  mDataLen = 0;
  mData = (uint8_t*)malloc(mDataReserved);
}
Stack::~Stack() {
  free(mData);
  mData = nullptr;
}
void Stack::ensureSpace(size_t additionalLen) {
  if(mDataReserved <= mDataLen + additionalLen) {
    mDataReserved *= 2;
    mData = (uint8_t*)realloc(mData, mDataReserved);
  }
}

}