#include <cassert>
#include "samal_lib/VM.hpp"
#include "samal_lib/Instruction.hpp"
#include <cstring>
#include <cstdlib>

namespace samal {

VM::VM(Program program)
    : mProgram(std::move(program)) {

}
std::vector<uint8_t> VM::run(const std::string &functionName, const std::vector<uint8_t> &initialStack) {
  mStack.push(initialStack);
  int32_t id = 0;
  auto functionOrNot = mProgram.functions.find(functionName);
  if(functionOrNot == mProgram.functions.end()) {
    throw std::runtime_error{"Function " + functionName + " not found!"};
  }
  mIp = functionOrNot->second.offset;
  mFunctionDepth = 1;
  auto dump = mStack.dump();
  printf("Dump:\n%s\n", dump.c_str());
  while(mFunctionDepth > 0) {
    auto ret = interpretInstruction();

    auto dump = mStack.dump();
    printf("Dump:\n%s\n", dump.c_str());
    if(!ret) {
      return mStack.moveData();
    }
  }
  assert(false);
}
bool VM::interpretInstruction() {
  bool incIp = true;
  auto ins = static_cast<Instruction>(mProgram.code.at(mIp));
  printf("Executing instruction %i: %s\n", static_cast<int>(ins), instructionToString(ins));
  switch(ins) {
    case Instruction::PUSH_4:
#ifdef x86_64_BIT_MODE
      assert(false);
#endif
      mStack.push(&mProgram.code.at(mIp + 1), 4);
      break;
    case Instruction::PUSH_8:
      mStack.push(&mProgram.code.at(mIp + 1), 8);
      break;
    case Instruction::REPUSH_FROM_N:
      mStack.repush(*(int32_t*)&mProgram.code.at(mIp + 5), *(int32_t*)&mProgram.code.at(mIp + 1));
      break;
    case Instruction::REPUSH_N:
      mStack.repush(0, *(int32_t*)&mProgram.code.at(mIp + 1));
      break;
    case Instruction::COMPARE_LESS_THAN_I32: {
#ifdef x86_64_BIT_MODE
      auto lhs = *(int32_t*)mStack.get(16);
      auto rhs = *(int32_t*)mStack.get(8);
      mStack.pop(16);
      int64_t res = lhs < rhs;
      mStack.push(&res, 8);
#else
      auto lhs = *(int32_t*)mStack.get(8);
      auto rhs = *(int32_t*)mStack.get(4);
      mStack.pop(8);
      bool res = lhs < rhs;
      mStack.push(&res, 1);
#endif
      break;
    }
    case Instruction::JUMP_IF_FALSE: {
#ifdef x86_64_BIT_MODE
      auto val = *(bool*)mStack.get(8);
      mStack.pop(8);
      if(!val) {
        mIp = *(int32_t*)&mProgram.code.at(mIp + 1);
        incIp = false;
      }
#else
      auto val = *(bool*)mStack.get(1);
      mStack.pop(1);
      if(!val) {
        mIp = *(int32_t*)&mProgram.code.at(mIp + 1);
        incIp = false;
      }
#endif
      break;
    }
    case Instruction::JUMP: {
      mIp = *(int32_t*)&mProgram.code.at(mIp + 1);
      incIp = false;
      break;
    }
    case Instruction::SUB_I32: {
#ifdef x86_64_BIT_MODE
      auto lhs = *(int32_t*)mStack.get(16);
      auto rhs = *(int32_t*)mStack.get(8);
      int64_t res = lhs - rhs;
      mStack.pop(16);
      mStack.push(&res, 8);
#else
      auto lhs = *(int32_t*)mStack.get(8);
      auto rhs = *(int32_t*)mStack.get(4);
      int32_t res = lhs - rhs;
      mStack.pop(8);
      mStack.push(&res, 4);
#endif
      break;
    }
    case Instruction::ADD_I32: {
#ifdef x86_64_BIT_MODE
      auto lhs = *(int32_t*)mStack.get(16);
      auto rhs = *(int32_t*)mStack.get(8);
      int64_t res = lhs + rhs;
      mStack.pop(16);
      mStack.push(&res, 8);
#else
      auto lhs = *(int32_t*)mStack.get(8);
      auto rhs = *(int32_t*)mStack.get(4);
      int32_t res = lhs + rhs;
      mStack.pop(8);
      mStack.push(&res, 4);
#endif
      break;
    }
    case Instruction::POP_N_BELOW: {
      mStack.popBelow(*(int32_t *) &mProgram.code.at(mIp + 5),
                      *(int32_t *) &mProgram.code.at(mIp + 1));
      break;
    }
    case Instruction::CALL: {
      auto offset = *(int32_t*)&mProgram.code.at(mIp + 1);
      auto newIp = *(int32_t*)mStack.get(offset + 8);
      // save old values to stack
      *(int32_t*)mStack.get(offset + 8) = mIp + instructionToWidth(Instruction::CALL);
      mIp = newIp;
      mFunctionDepth += 1;
      incIp = false;
      break;
    }
    case Instruction::RETURN: {
      if(mFunctionDepth == 1) {
        return false;
      }
      mFunctionDepth -= 1;
      auto offset = *(int32_t*)&mProgram.code.at(mIp + 1);
      mIp = *(int32_t*)mStack.get(offset + 8);
      mStack.popBelow(offset, 8);
      incIp = false;
      break;
    }
    default:
      fprintf(stderr, "Unhandled instruction %i: %s\n", static_cast<int>(ins), instructionToString(ins));
      assert(false);
  }
  mIp += instructionToWidth(ins) * incIp;
  return true;
}

struct JitReturn {
  int32_t ip; // lower 4 bytes
  int32_t stackSize; // upper 4 bytes
};

class JitCode : public Xbyak::CodeGenerator {
 public:
  JitCode(const std::vector<uint8_t>& instructions, size_t offset) {
    // prelude
    push(rbx);
    push(rsp);
    push(rbp);
    push(r12);
    push(r13);
    push(r14);
    push(r15);
    mov(r15, rsp);

    // ip
    const auto& ip = r11;
    mov(ip, rdi);
    // stack ptr
    const auto& stackPtr = r12;
    mov(stackPtr, rsi);
    // stack size
    const auto& stackSize = r13;
    mov(stackSize, rdx);

    // copy stack in
    //    init copy
    mov(rcx, stackSize);
    mov(rsi, stackPtr);
    mov(rdi, rsp);
    sub(rdi, stackSize);
    cld();
    //    copy
    rep();
    movsb();
    //    adjust rsp
    sub(rsp, stackSize);


    // Start executing some Code!
    pop(rax);
    push(rax);
    inc(rax);
    push(rax);


    // calculate new stack size
    mov(stackSize, r15);
    sub(stackSize, rsp);

    // copy stack out
    //    init copy
    mov(rcx, stackSize);
    mov(rsi, rsp);
    mov(rdi, stackPtr);
    cld();
    //    copy
    rep();
    movsb();
    //    adjust rsp
    add(rsp, stackSize);

    // do some magic to put but stackSize and ip in the rax register
    mov(rax, stackSize);
    sal(rax, 32);
    mov(rbx, ip);
    or_(rax, rbx);

    // restore registers & stack
    mov(rsp, r15);
    pop(r15);
    pop(r14);
    pop(r13);
    pop(r12);
    pop(rbp);
    pop(rsp);
    pop(rbx);
    ret();
  }
 private:
};

bool VM::jitCompileAndRunInstruction() {
  JitCode code{mProgram.code, mIp};
  static_assert(sizeof(JitReturn) == 8);
  // reverse stack order to match the fact that the cpu stack grows downwards but ours grows upwards
  auto reverseStack = [&] {
    for(int i = 0; i < mStack.getSize() / 16; ++i) {
      auto castedPtr = (int64_t*)mStack.getBasePtr();
      auto temp = castedPtr[i];
      castedPtr[i] = castedPtr[mStack.getSize() / 8 - i - 1];
      castedPtr[mStack.getSize() / 8 - i - 1] = temp;
    }
  };
  reverseStack();
  JitReturn ret = code.getCode<JitReturn(*)(int32_t, uint8_t*, int32_t)>()(mIp, mStack.getBasePtr(), mStack.getSize());
  mStack.setSize(ret.stackSize);
  mIp = ret.ip;
  // and reset back to original
  reverseStack();
  return false;
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
  for(size_t i = 0; i < mDataLen;) {
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
uint8_t *Stack::getBasePtr() {
  return mData;
}
size_t Stack::getSize() {
  return mDataLen;
}
void Stack::setSize(size_t val) {
  assert(val <= mDataReserved);
  mDataLen = val;
}

}