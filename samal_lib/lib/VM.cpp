#include "samal_lib/VM.hpp"
#include "samal_lib/Instruction.hpp"
#include "samal_lib/Util.hpp"
#include <cassert>
#include <cstdlib>
#include <cstring>

namespace samal {

struct JitReturn {
    int32_t ip;        // lower 4 bytes
    int32_t stackSize; // upper 4 bytes
};

[[maybe_unused]] static constexpr bool isJittable(Instruction i) {
    switch (i) {
    case Instruction::PUSH_8:
    case Instruction::ADD_I32:
    case Instruction::SUB_I32:
    case Instruction::COMPARE_LESS_THAN_I32:
    case Instruction::COMPARE_LESS_EQUAL_THAN_I32:
    case Instruction::COMPARE_MORE_THAN_I32:
    case Instruction::COMPARE_MORE_EQUAL_THAN_I32:
    case Instruction::REPUSH_FROM_N:
    case Instruction::JUMP:
    case Instruction::JUMP_IF_FALSE:
    case Instruction::POP_N_BELOW:
    case Instruction::CALL:
    case Instruction::RETURN:
        return true;
    default:
        return false;
    }
}

class JitCode : public Xbyak::CodeGenerator {
public:
    explicit JitCode(const std::vector<uint8_t>& instructions)
    : Xbyak::CodeGenerator(4096 * 4){
        setDefaultJmpNEAR(true);
        // prelude
        push(rbx);
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

        // always just contain a zero or a one respectively
        const auto& oneRegister = r8;
        mov(oneRegister, 1);

        const auto& tableRegister = r9;
        mov(tableRegister, "JumpTable");

        // copy stack in
        //    init copy
        mov(rcx, stackSize);
        mov(rsi, stackPtr);
        mov(rdi, rsp);
        sub(rdi, stackSize);
        cld(); // up
        //    copy
        rep();
        movsb();
        //    adjust rsp
        sub(rsp, stackSize);


        std();
        auto jumpWithIp = [&] {
          jmp(ptr [tableRegister + ip * sizeof(void*)]);
        };
        jumpWithIp();

        L("Code");
        std::vector<std::pair<uint32_t, Xbyak::Label>> labels;
        // Start executing some Code!
        for (size_t i = 0; i < instructions.size();) {
            auto ins = static_cast<Instruction>(instructions.at(i));
            auto nextInstruction = [&] {
                return static_cast<Instruction>(instructions.at(i + instructionToWidth(ins)));
            };
            auto nextInstructionWidth = [&] {
                return instructionToWidth(nextInstruction());
            };
            bool shouldAutoIncrement = true;
            labels.emplace_back(std::make_pair((uint32_t)i, L()));
            switch (ins) {
            case Instruction::PUSH_8:
                if (nextInstruction() == Instruction::SUB_I32) {
                    pop(rbx);
                    sub(rbx, *(uint64_t*)&instructions.at(i + 1));
                    push(rbx);
                    add(ip, instructionToWidth(ins) + nextInstructionWidth());
                    i += instructionToWidth(ins) + nextInstructionWidth();
                    shouldAutoIncrement = false;
                } else if (nextInstruction() == Instruction::ADD_I32) {
                    pop(rbx);
                    add(rbx, *(uint64_t*)&instructions.at(i + 1));
                    push(rbx);
                    add(ip, instructionToWidth(ins) + nextInstructionWidth());
                    i += instructionToWidth(ins) + nextInstructionWidth();
                    shouldAutoIncrement = false;
                } else {
                    mov(rax, *(uint64_t*)&instructions.at(i + 1));
                    push(rax);
                }
                break;
            case Instruction::ADD_I32:
                pop(rax);
                pop(rbx);
                add(rbx, rax);
                push(rbx);
                break;
            case Instruction::SUB_I32:
                pop(rax);
                pop(rbx);
                sub(rbx, rax);
                push(rbx);
                break;
            case Instruction::COMPARE_LESS_THAN_I32:
                pop(rax);
                pop(rbx);
                cmp(rbx, rax);
                mov(rax, 0);
                cmovl(rax, oneRegister);
                push(rax);
                break;
            case Instruction::COMPARE_LESS_EQUAL_THAN_I32:
                pop(rax);
                pop(rbx);
                cmp(rbx, rax);
                mov(rax, 0);
                cmovle(rax, oneRegister);
                push(rax);
                break;
            case Instruction::COMPARE_MORE_THAN_I32:
                pop(rax);
                pop(rbx);
                cmp(rbx, rax);
                mov(rax, 0);
                cmovg(rax, oneRegister);
                push(rax);
                break;
            case Instruction::COMPARE_MORE_EQUAL_THAN_I32:
                pop(rax);
                pop(rbx);
                cmp(rbx, rax);
                mov(rax, 0);
                cmovge(rax, oneRegister);
                push(rax);
                break;
            case Instruction::REPUSH_FROM_N: {
                int32_t repushLen = *(uint32_t*)&instructions.at(i + 1);
                int32_t repushOffset = *(uint32_t*)&instructions.at(i + 5);
                //    init copy
                assert(repushLen % 8 == 0);
                mov(rsi, rsp);
                add(rsi, repushOffset);
                sub(rsp, repushLen);
                mov(rdi, rsp);
                //    copy
                for (int j = 0; j < repushLen / 8; ++j) {
                    movsq();
                }
                break;
            }
            case Instruction::POP_N_BELOW: {
                int32_t popLen = *(uint32_t*)&instructions.at(i + 1);
                int32_t popOffset = *(uint32_t*)&instructions.at(i + 5);
                //    init copy
                assert(popOffset % 8 == 0);
                mov(rsi, rsp);
                add(rsi, popOffset - 8);

                mov(rdi, rsp);
                add(rdi, popOffset + popLen - 8);

                //    copy
                for (int j = 0; j < popOffset / 8; ++j) {
                    movsq();
                }

                add(rsp, popLen);
                break;
            }
            case Instruction::JUMP: {
                auto p = *(int32_t*)&instructions.at(i + 1);
                mov(ip, p);
                jumpWithIp();
                break;
            }
            case Instruction::JUMP_IF_FALSE: {
                pop(rax);
                add(ip, instructionToWidth(ins));
                mov(rbx, *(uint32_t*)&instructions.at(i + 1));
                cmp(rax, 0);
                cmove(ip, rbx);
                jumpWithIp();
                break;
            }
            case Instruction::CALL: {
                int32_t callInfoOffset = *(uint32_t*)&instructions.at(i + 1);
                mov(rax, rsp);
                add(rax, callInfoOffset);
                mov(rbx, qword[rax]);
                add(ip, instructionToWidth(ins));
                mov(qword[rax], ip);

                mov(ip, rbx);
                jumpWithIp();
                break;
            }
            case Instruction::RETURN: {
                int32_t returnInfoOffset = *(uint32_t*)&instructions.at(i + 1);
                mov(rax, rsp);
                add(rax, returnInfoOffset);
                mov(ip, qword[rax]);

                // pop below
                {
                    int32_t popLen = 8;
                    int32_t popOffset = returnInfoOffset;
                    //    init copy
                    assert(popOffset % 8 == 0);

                    mov(rsi, rsp);
                    add(rsi, popOffset - 8);

                    mov(rdi, rsp);
                    add(rdi, popOffset + popLen - 8);

                    //    copy
                    for (int j = 0; j < popOffset / 8; ++j) {
                        movsq();
                    }

                    add(rsp, popLen);
                }
                // TODO replace last return address with another special value that we can integrate into the jump table
                cmp(ip, 0x42424242);
                jge("AfterJumpTable");

                jumpWithIp();
                break;
            }
            default:
                // we hit an instruction that we don't know, so exit the jit
                labels.erase(--labels.end());
                jmp("AfterJumpTable");
                break;
            }
            if (shouldAutoIncrement) {
                i += instructionToWidth(ins);
                add(ip, instructionToWidth(ins));
            }
        }

        jmp("AfterJumpTable");

        /*L("JumpTable");
        for (auto& label : labels) {
            mov(rax, label.first);
            cmp(rax, ip);
            je(label.second);
        }*/

        L("JumpTable");
        for(size_t i = 0; i <= instructions.size(); ++i) {
          bool labelExists = false;
          for(auto& label: labels) {
            if(label.first == i) {
              labelExists = true;
              putL(label.second);
              break;
            }
          }
          if(!labelExists) {
            putL("AfterJumpTable");
          }
        }
        L("AfterJumpTable");

        assert(!hasUndefinedLabel());

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
        mov(ebx, ebx);
        or_(rax, rbx);

        // restore registers & stack
        mov(rsp, r15);
        pop(r15);
        pop(r14);
        pop(r13);
        pop(r12);
        pop(rbp);
        pop(rbx);
        ret();
    }
};

VM::VM(Program program)
: mProgram(std::move(program)) {
    mCompiledCode = std::make_unique<JitCode>(mProgram.code);
}
std::vector<uint8_t> VM::run(const std::string& functionName, const std::vector<uint8_t>& initialStack) {
    mStack.clear();
    mStack.push(initialStack);
    auto functionOrNot = mProgram.functions.find(functionName);
    if (functionOrNot == mProgram.functions.end()) {
        throw std::runtime_error { "Function " + functionName + " not found!" };
    }
    mIp = functionOrNot->second.offset;
    mMainFunctionReturnTypeSize = functionOrNot->second.returnTypeSize;
#ifdef _DEBUG
    auto dump = mStack.dump();
    printf("Dump:\n%s\n", dump.c_str());
#endif
    while (true) {
        // first try to jit as many instructions as possible
        {
#ifdef _DEBUG
            printf("Executing jit...\n");
#endif
            std::reverse((int64_t*)mStack.getBasePtr(), (int64_t*)(mStack.getBasePtr() + mStack.getSize()));
            JitReturn ret = mCompiledCode->getCode<JitReturn (*)(int32_t, uint8_t*, int32_t)>()(mIp, mStack.getBasePtr(), mStack.getSize());
            mStack.setSize(ret.stackSize);
            mIp = ret.ip;
            std::reverse((int64_t*)mStack.getBasePtr(), (int64_t*)(mStack.getBasePtr() + mStack.getSize()));
            if (mIp == 0x42424242) {
#ifdef _DEBUG
                auto dump = mStack.dump();
                printf("Dump:\n%s\n", dump.c_str());
#endif
                return mStack.moveData();
            }
#ifdef _DEBUG
            auto dump = mStack.dump();
            printf("New ip: %u\n", mIp);
            printf("Dump:\n%s\n", dump.c_str());
#endif
        }
        // then run one through the interpreter
        // TODO run multiple instructions at once if multiple instructions in a row can't be jitted
        auto ret = interpretInstruction();
#ifdef _DEBUG
        auto dump = mStack.dump();
        printf("Dump:\n%s\n", dump.c_str());
#endif
        if (!ret) {
            return mStack.moveData();
        }
    }
}
bool VM::interpretInstruction() {
    bool incIp = true;
    auto ins = static_cast<Instruction>(mProgram.code.at(mIp));
#ifdef _DEBUG
    printf("Executing instruction %i: %s\n", static_cast<int>(ins), instructionToString(ins));
#endif
    switch (ins) {
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
        if (!val) {
            mIp = *(int32_t*)&mProgram.code.at(mIp + 1);
            incIp = false;
        }
#else
        auto val = *(bool*)mStack.get(1);
        mStack.pop(1);
        if (!val) {
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
        mStack.popBelow(*(int32_t*)&mProgram.code.at(mIp + 5),
            *(int32_t*)&mProgram.code.at(mIp + 1));
        break;
    }
    case Instruction::CALL: {
        auto offset = *(int32_t*)&mProgram.code.at(mIp + 1);
        auto newIp = *(int32_t*)mStack.get(offset + 8);
        // save old values to stack
        *(int32_t*)mStack.get(offset + 8) = mIp + instructionToWidth(Instruction::CALL);
        mIp = newIp;
        incIp = false;
        break;
    }
    case Instruction::RETURN: {
        auto offset = *(int32_t*)&mProgram.code.at(mIp + 1);
        mIp = *(int32_t*)mStack.get(offset + 8);
        mStack.popBelow(offset, 8);
        incIp = false;
        if (mStack.getSize() == mMainFunctionReturnTypeSize) {
            return false;
        }
        break;
    }
    default:
        fprintf(stderr, "Unhandled instruction %i: %s\n", static_cast<int>(ins), instructionToString(ins));
        assert(false);
    }
    mIp += instructionToWidth(ins) * incIp;
    return true;
}
VM::~VM() = default;
void Stack::push(const std::vector<uint8_t>& data) {
    push(data.data(), data.size());
}
void Stack::push(const void* data, size_t len) {
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
void* Stack::get(size_t offset) {
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
    for (size_t i = 0; i < mDataLen;) {
        uint8_t val = mData[i];
        ret += std::to_string(val) + " ";
        ++i;
        if (i > 0 && i % 4 == 0) {
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
    if (mDataReserved <= mDataLen + additionalLen) {
        mDataReserved *= 2;
        mData = (uint8_t*)realloc(mData, mDataReserved);
    }
}
uint8_t* Stack::getBasePtr() {
    return mData;
}
size_t Stack::getSize() const {
    return mDataLen;
}
void Stack::setSize(size_t val) {
    assert(val <= mDataReserved);
    mDataLen = val;
}
void Stack::clear() {
    mDataLen = 0;
}

}