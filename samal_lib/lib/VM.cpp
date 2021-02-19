#include "samal_lib/VM.hpp"
#include "samal_lib/ExternalVMValue.hpp"
#include "samal_lib/Instruction.hpp"
#include "samal_lib/StackInformationTree.hpp"
#include "samal_lib/Util.hpp"
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cstring>

namespace samal {

#ifdef SAMAL_ENABLE_JIT
struct JitReturn {
    int32_t ip;                   // lower 4 bytes of rax
    int32_t stackSize;            // upper 4 bytes of rax
    int32_t nativeFunctionToCall; // lower 4 bytes of rdx
};

class JitCode : public Xbyak::CodeGenerator {
public:
    explicit JitCode(const std::vector<uint8_t>& instructions)
    : Xbyak::CodeGenerator(4096 * 4, Xbyak::AutoGrow) {
        setDefaultJmpNEAR(true);
        // prelude
        push(rbx);
        push(rbp);
        push(r10);
        push(r11);
        push(r12);
        push(r13);
        push(r14);
        push(r15);
        mov(r15, rsp);

        // ip
        const auto& ip = r11;
        [[maybe_unused]] const auto& ip32 = r11d;
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

        const auto& nativeFunctionIdRegister = r10;
        mov(nativeFunctionIdRegister, 0);

        mov(rsp, stackPtr);

        auto jumpWithIp = [&] {
            jmp(ptr[tableRegister + ip * sizeof(void*)]);
        };
        jumpWithIp();

        L("Code");
        std::vector<std::pair<uint32_t, Xbyak::Label>> labels;
        // Start executing some Code!
        for(size_t i = 0; i < instructions.size();) {
            auto ins = static_cast<Instruction>(instructions.at(i));
            auto nextInstruction = [&] {
                return static_cast<Instruction>(instructions.at(i + instructionToWidth(ins)));
            };
            auto nextInstructionWidth = [&] {
                return instructionToWidth(nextInstruction());
            };
            bool shouldAutoIncrement = true;
            labels.emplace_back(std::make_pair((uint32_t)i, L()));
            switch(ins) {
            case Instruction::PUSH_8: {
                auto amount = *(uint64_t*)&instructions.at(i + 1);
                if(nextInstruction() == Instruction::SUB_I32) {
                    sub(qword[rsp], amount);
                    add(ip, instructionToWidth(ins) + nextInstructionWidth());
                    i += instructionToWidth(ins) + nextInstructionWidth();
                    shouldAutoIncrement = false;
                } else if(nextInstruction() == Instruction::ADD_I32) {
                    add(qword[rsp], amount);
                    add(ip, instructionToWidth(ins) + nextInstructionWidth());
                    i += instructionToWidth(ins) + nextInstructionWidth();
                    shouldAutoIncrement = false;
                } else if(nextInstruction() == Instruction::DIV_I32) {
                    pop(rax);
                    mov(rbx, amount);
                    cdq();
                    idiv(ebx);
                    push(rax);
                    add(ip, instructionToWidth(ins) + nextInstructionWidth());
                    i += instructionToWidth(ins) + nextInstructionWidth();
                    shouldAutoIncrement = false;
                    // TODO optimize MUL_I32, DIV_I64 and MUL_I64 as well
                } else {
                    mov(rax, *(uint64_t*)&instructions.at(i + 1));
                    push(rax);
                }
                break;
            }
            case Instruction::ADD_I32:
                pop(rax);
                add(dword[rsp], eax);
                break;
            case Instruction::SUB_I32:
                pop(rax);
                sub(dword[rsp], eax);
                break;
            case Instruction::MUL_I32:
                pop(rbx);
                imul(ebx, qword[rsp]);
                mov(qword[rsp], rbx);
                break;
            case Instruction::DIV_I32:
                pop(rbx);
                pop(rax);
                cdq();
                idiv(ebx);
                push(rax);
                break;
            case Instruction::COMPARE_LESS_THAN_I32:
                pop(rax);
                pop(rbx);
                cmp(ebx, eax);
                mov(rax, 0);
                cmovl(rax, oneRegister);
                push(rax);
                break;
            case Instruction::COMPARE_LESS_EQUAL_THAN_I32:
                pop(rax);
                pop(rbx);
                cmp(ebx, eax);
                mov(rax, 0);
                cmovle(rax, oneRegister);
                push(rax);
                break;
            case Instruction::COMPARE_MORE_THAN_I32:
                pop(rax);
                pop(rbx);
                cmp(ebx, eax);
                mov(rax, 0);
                cmovg(rax, oneRegister);
                push(rax);
                break;
            case Instruction::COMPARE_MORE_EQUAL_THAN_I32:
                pop(rax);
                pop(rbx);
                cmp(ebx, eax);
                mov(rax, 0);
                cmovge(rax, oneRegister);
                push(rax);
                break;
            case Instruction::ADD_I64:
                pop(rax);
                add(qword[rsp], rax);
                break;
            case Instruction::SUB_I64:
                pop(rax);
                sub(qword[rsp], rax);
                break;
            case Instruction::MUL_I64:
                pop(rbx);
                imul(rbx, qword[rsp]);
                mov(qword[rsp], rbx);
                break;
            case Instruction::DIV_I64:
                pop(rbx);
                pop(rax);
                cqo();
                idiv(rbx);
                push(rax);
                break;
            case Instruction::COMPARE_LESS_THAN_I64:
                pop(rax);
                pop(rbx);
                cmp(rbx, rax);
                mov(rax, 0);
                cmovl(rax, oneRegister);
                push(rax);
                break;
            case Instruction::COMPARE_LESS_EQUAL_THAN_I64:
                pop(rax);
                pop(rbx);
                cmp(rbx, rax);
                mov(rax, 0);
                cmovle(rax, oneRegister);
                push(rax);
                break;
            case Instruction::COMPARE_MORE_THAN_I64:
                pop(rax);
                pop(rbx);
                cmp(rbx, rax);
                mov(rax, 0);
                cmovg(rax, oneRegister);
                push(rax);
                break;
            case Instruction::COMPARE_MORE_EQUAL_THAN_I64:
                pop(rax);
                pop(rbx);
                cmp(rbx, rax);
                mov(rax, 0);
                cmovge(rax, oneRegister);
                push(rax);
                break;
            case Instruction::LOGICAL_OR:
                pop(rax);
                or_(qword[rsp], rax);
                break;
            case Instruction::REPUSH_FROM_N: {
                int32_t repushLen = *(uint32_t*)&instructions.at(i + 1);
                int32_t repushOffset = *(uint32_t*)&instructions.at(i + 5);
                assert(repushLen % 8 == 0);
                for(int j = 0; j < repushLen / 8; ++j) {
                    mov(rax, qword[rsp + (repushOffset + repushLen - 8)]);
                    push(rax);
                }
                break;
            }
            case Instruction::POP_N_BELOW: {
                int32_t popLen = *(uint32_t*)&instructions.at(i + 1);
                int32_t popOffset = *(uint32_t*)&instructions.at(i + 5);
                assert(popOffset % 8 == 0);
                for(int j = popOffset / 8 - 1; j >= 0; --j) {
                    mov(rax, ptr[rsp + (j * 8)]);
                    mov(ptr[rsp + (j * 8 + popLen)], rax);
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
                test(rax, rax);
                cmovz(ip, rbx);
                jumpWithIp();
                break;
            }
            case Instruction::CALL: {
                int32_t callInfoOffset = *(uint32_t*)&instructions.at(i + 1);
                mov(rax, rsp);
                add(rax, callInfoOffset);
                // rax points to the location of the function id/ptr
                mov(rbx, qword[rax]);
                // rbx contains the new ip if it's a normal function and a ptr if it's a lambda
                test(bl, 1);
                Xbyak::Label normalOrNativeFunctionLocation;
                Xbyak::Label end;
                jnz(normalOrNativeFunctionLocation);
                // lambda
                // setup rsi
                mov(rsi, rbx);
                add(rsi, 16);
                // rsi points to the start of the buffer in ram
                // extract length & ip
                mov(ecx, dword[rbx]);
                // rcx contains the length of the lambda data
                mov(ebx, dword[rbx + 4]);
                // rbx contains the ip we should jump to
                sub(rsp, rcx);
                mov(rdi, rsp);
                // rdi points to the end of destination on the stack
                cld();
                rep();
                movsb();

                jmp(end);

                L(normalOrNativeFunctionLocation);
                // normal function or native function
                Xbyak::Label normalFunctionLocation;
                cmp(bl, 3);
                jne(normalFunctionLocation);
                // native function
                sar(rbx, 32);
                mov(nativeFunctionIdRegister, rbx);
                inc(nativeFunctionIdRegister);
                add(ip, instructionToWidth(ins));
                jmp("AfterJumpTable");
                // note: we never actually execute the code below because of the jmp
                L(normalFunctionLocation);
                // normal/default function
                sar(rbx, 32);
                L(end);
                add(ip, instructionToWidth(ins));
                mov(dword[rax + 4], ip32);

                mov(ip, rbx);
                jumpWithIp();
                break;
            }
            case Instruction::RETURN: {
                int32_t returnInfoOffset = *(uint32_t*)&instructions.at(i + 1);
                mov(ip, qword[rsp + returnInfoOffset]);
                sar(ip, 32);

                // pop below
                {
                    int32_t popLen = 8;
                    int32_t popOffset = returnInfoOffset;
                    assert(popOffset % 8 == 0);
                    for(int j = popOffset / 8 - 1; j >= 0; --j) {
                        mov(rax, ptr[rsp + (j * 8)]);
                        mov(ptr[rsp + (j * 8 + popLen)], rax);
                    }
                    add(rsp, popLen);
                }

                jumpWithIp();
                break;
            }
            case Instruction::LIST_GET_TAIL: {
                cmp(qword[rsp], 0);
                Xbyak::Label after;
                je(after);
                mov(rax, qword[rsp]);
                mov(rax, qword[rax]);
                mov(qword[rsp], rax);
                L(after);
                break;
            }
            case Instruction::LOAD_FROM_PTR: {
                auto sizeOfElement = *(int32_t*)&instructions.at(i + 1);
                auto offset = *(int32_t*)&instructions.at(i + 5);

                cmp(qword[rsp], 0);
                Xbyak::Label after;
                je("AfterJumpTable");

                pop(rax);
                assert(sizeOfElement % 8 == 0);
                for(int32_t i = 0; i < sizeOfElement / 8; ++i) {
                    mov(rbx, qword[rax + (8 * i + offset)]);
                    push(rbx);
                }
                L(after);
                break;
            }
            case Instruction::IS_LIST_EMPTY: {
                mov(rbx, 0);
                cmp(qword[rsp], 0);
                cmove(rbx, oneRegister);
                mov(qword[rsp], rbx);
                break;
            }
            default:
                // we hit an instruction that we don't know, so exit the jit
                labels.erase(--labels.end());
                jmp("AfterJumpTable");
                break;
            }
            if(shouldAutoIncrement) {
                i += instructionToWidth(ins);
                add(ip, instructionToWidth(ins));
            }
        }

        jmp("AfterJumpTable");

        L("JumpTable");
        for(size_t i = 0; i <= instructions.size(); ++i) {
            bool labelExists = false;
            for(auto& label : labels) {
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

        mov(r14, stackPtr);
        add(r14, stackSize);
        // r14 now points to the upper end of stack

        // calculate new stack size
        sub(r14, rsp);
        mov(stackSize, r14);
        mov(rsp, r15);

        // do some magic to put but stackSize and ip in the rax register
        mov(rax, stackSize);
        sal(rax, 32);
        mov(rbx, ip);
        mov(ebx, ebx);
        or_(rax, rbx);

        // put native function id in the rdx register
        mov(rdx, nativeFunctionIdRegister);

        // restore registers & stack
        mov(rsp, r15);
        pop(r15);
        pop(r14);
        pop(r13);
        pop(r12);
        pop(r11);
        pop(r10);
        pop(rbp);
        pop(rbx);
        ret();

        readyRE();
    }
};
#else
class JitCode {
};
#endif

VM::VM(Program program)
: mProgram(std::move(program)), mGC(*this) {
#ifdef SAMAL_ENABLE_JIT
    mCompiledCode = std::make_unique<JitCode>(mProgram.code);
#endif
}
ExternalVMValue VM::run(const std::string& functionName, std::vector<uint8_t> initialStack) {
    mStack.clear();
    mStack.push(initialStack);
    Program::Function* function{ nullptr };
    for(auto& func : mProgram.functions) {
        if(func.name == functionName) {
            function = &func;
        }
    }
    if(!function) {
        throw std::runtime_error{ "Function " + functionName + " not found!" };
    }
    mIp = function->offset;
    auto& returnType = function->type.getFunctionTypeInfo().first;
#ifdef _DEBUG
    auto dump = mStack.dump();
    printf("Dump:\n%s\n", dump.c_str());
#endif
    while(true) {
#ifdef SAMAL_ENABLE_JIT
        // first try to jit as many instructions as possible
        {
#    ifdef _DEBUG
            printf("Executing jit...\n");
#    endif
            JitReturn ret = mCompiledCode->getCode<JitReturn (*)(int32_t, uint8_t*, int32_t)>()(mIp, mStack.getTopPtr(), mStack.getSize());
            mStack.setSize(ret.stackSize);
            mIp = ret.ip;
            if(mIp == static_cast<int32_t>(mProgram.code.size())) {
#    ifdef _DEBUG
                auto dump = mStack.dump();
                printf("Dump:\n%s\n", dump.c_str());
#    endif
                return ExternalVMValue::wrapStackedValue(returnType, *this, 0);
            }
#    ifdef _DEBUG
            auto dump = mStack.dump();
            printf("New ip: %u\n", mIp);
            printf("Dump:\n%s\n", dump.c_str());
#    endif
            if(ret.nativeFunctionToCall) {
                auto newIp = mIp;
                mIp -= instructionToWidth(Instruction::CALL);
                execNativeFunction(ret.nativeFunctionToCall - 1);
                mIp = newIp;
                continue;
            }
        }
#endif
        // then run one through the interpreter
        // TODO run multiple instructions at once if multiple instructions in a row can't be jitted
        auto ret = interpretInstruction();
#ifdef _DEBUG
        auto stackDump = mStack.dump();
        printf("Stack:\n%s\n", stackDump.c_str());
#endif
        if(!ret) {
            return ExternalVMValue::wrapStackedValue(returnType, *this, 0);
        }
    }
}
bool VM::interpretInstruction() {
    bool incIp = true;
    auto ins = static_cast<Instruction>(mProgram.code.at(mIp));
#ifdef x86_64_BIT_MODE
    constexpr int32_t BOOL_SIZE = 8;
#else
    constexpr int32_t BOOL_SIZE = 1;
#endif
#ifdef _DEBUG
    // dump
    auto varDump = dumpVariablesOnStack();
    printf("%s", varDump.c_str());
    printf("Executing instruction %i: %s\n", static_cast<int>(ins), instructionToString(ins));
#endif
    switch(ins) {
    case Instruction::PUSH_1:
        mStack.push(&mProgram.code.at(mIp + 1), 1);
        break;
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
    case Instruction::JUMP_IF_FALSE: {
#ifdef x86_64_BIT_MODE
        auto val = *(bool*)mStack.get(0);
        mStack.pop(8);
        if(!val) {
            mIp = *(int32_t*)&mProgram.code.at(mIp + 1);
            incIp = false;
        }
#else
        auto val = *(bool*)mStack.get(0);
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
        auto lhs = *(int32_t*)mStack.get(8);
        auto rhs = *(int32_t*)mStack.get(0);
        int64_t res = lhs - rhs;
        mStack.pop(16);
        mStack.push(&res, 8);
#else
        auto lhs = *(int32_t*)mStack.get(4);
        auto rhs = *(int32_t*)mStack.get(0);
        int32_t res = lhs - rhs;
        mStack.pop(8);
        mStack.push(&res, 4);
#endif
        break;
    }
    case Instruction::ADD_I32: {
#ifdef x86_64_BIT_MODE
        auto lhs = *(int32_t*)mStack.get(8);
        auto rhs = *(int32_t*)mStack.get(0);
        int64_t res = lhs + rhs;
        mStack.pop(16);
        mStack.push(&res, 8);
#else
        auto lhs = *(int32_t*)mStack.get(4);
        auto rhs = *(int32_t*)mStack.get(0);
        int32_t res = lhs + rhs;
        mStack.pop(8);
        mStack.push(&res, 4);
#endif
        break;
    }
    case Instruction::MUL_I32: {
#ifdef x86_64_BIT_MODE
        auto lhs = *(int32_t*)mStack.get(8);
        auto rhs = *(int32_t*)mStack.get(0);
        int64_t res = lhs * rhs;
        mStack.pop(16);
        mStack.push(&res, 8);
#else
        auto lhs = *(int32_t*)mStack.get(4);
        auto rhs = *(int32_t*)mStack.get(0);
        int32_t res = lhs * rhs;
        mStack.pop(8);
        mStack.push(&res, 4);
#endif
        break;
    }
    case Instruction::DIV_I32: {
#ifdef x86_64_BIT_MODE
        auto lhs = *(int32_t*)mStack.get(8);
        auto rhs = *(int32_t*)mStack.get(0);
        int64_t res = lhs / rhs;
        mStack.pop(16);
        mStack.push(&res, 8);
#else
        auto lhs = *(int32_t*)mStack.get(4);
        auto rhs = *(int32_t*)mStack.get(0);
        int32_t res = lhs / rhs;
        mStack.pop(8);
        mStack.push(&res, 4);
#endif
        break;
    }
    case Instruction::MODULO_I32: {
#ifdef x86_64_BIT_MODE
        auto lhs = *(int32_t*)mStack.get(8);
        auto rhs = *(int32_t*)mStack.get(0);
        int64_t res = lhs % rhs;
        mStack.pop(16);
        mStack.push(&res, 8);
#else
        auto lhs = *(int32_t*)mStack.get(4);
        auto rhs = *(int32_t*)mStack.get(0);
        int32_t res = lhs % rhs;
        mStack.pop(8);
        mStack.push(&res, 4);
#endif
        break;
    }
    case Instruction::COMPARE_LESS_THAN_I32: {
#ifdef x86_64_BIT_MODE
        auto lhs = *(int32_t*)mStack.get(8);
        auto rhs = *(int32_t*)mStack.get(0);
        mStack.pop(16);
        int64_t res = lhs < rhs;
        mStack.push(&res, 8);
#else
        auto lhs = *(int32_t*)mStack.get(4);
        auto rhs = *(int32_t*)mStack.get(0);
        mStack.pop(8);
        bool res = lhs < rhs;
        mStack.push(&res, 1);
#endif
        break;
    }
    case Instruction::COMPARE_MORE_THAN_I32: {
#ifdef x86_64_BIT_MODE
        auto lhs = *(int32_t*)mStack.get(8);
        auto rhs = *(int32_t*)mStack.get(0);
        mStack.pop(16);
        int64_t res = lhs > rhs;
        mStack.push(&res, 8);
#else
        auto lhs = *(int32_t*)mStack.get(4);
        auto rhs = *(int32_t*)mStack.get(0);
        mStack.pop(8);
        bool res = lhs > rhs;
        mStack.push(&res, 1);
#endif
        break;
    }
    case Instruction::COMPARE_LESS_EQUAL_THAN_I32: {
#ifdef x86_64_BIT_MODE
        auto lhs = *(int32_t*)mStack.get(8);
        auto rhs = *(int32_t*)mStack.get(0);
        mStack.pop(16);
        int64_t res = lhs <= rhs;
        mStack.push(&res, 8);
#else
        auto lhs = *(int32_t*)mStack.get(4);
        auto rhs = *(int32_t*)mStack.get(0);
        mStack.pop(8);
        bool res = lhs <= rhs;
        mStack.push(&res, 1);
#endif
        break;
    }
    case Instruction::COMPARE_MORE_EQUAL_THAN_I32: {
#ifdef x86_64_BIT_MODE
        auto lhs = *(int32_t*)mStack.get(8);
        auto rhs = *(int32_t*)mStack.get(0);
        mStack.pop(16);
        int64_t res = lhs >= rhs;
        mStack.push(&res, 8);
#else
        auto lhs = *(int32_t*)mStack.get(4);
        auto rhs = *(int32_t*)mStack.get(0);
        mStack.pop(8);
        bool res = lhs >= rhs;
        mStack.push(&res, 1);
#endif
        break;
    }
    case Instruction::COMPARE_EQUALS_I32: {
#ifdef x86_64_BIT_MODE
        auto lhs = *(int32_t*)mStack.get(8);
        auto rhs = *(int32_t*)mStack.get(0);
        mStack.pop(16);
        int64_t res = lhs == rhs;
        mStack.push(&res, 8);
#else
        auto lhs = *(int32_t*)mStack.get(4);
        auto rhs = *(int32_t*)mStack.get(0);
        mStack.pop(8);
        bool res = lhs == rhs;
        mStack.push(&res, 1);
#endif
        break;
    }
    case Instruction::SUB_I64: {
        auto lhs = *(int64_t*)mStack.get(8);
        auto rhs = *(int64_t*)mStack.get(0);
        int64_t res = lhs - rhs;
        mStack.pop(16);
        mStack.push(&res, 8);
        break;
    }
    case Instruction::ADD_I64: {
        auto lhs = *(int64_t*)mStack.get(8);
        auto rhs = *(int64_t*)mStack.get(0);
        int64_t res = lhs + rhs;
        mStack.pop(16);
        mStack.push(&res, 8);
        break;
    }
    case Instruction::MUL_I64: {
        auto lhs = *(int64_t*)mStack.get(8);
        auto rhs = *(int64_t*)mStack.get(0);
        int64_t res = lhs * rhs;
        mStack.pop(16);
        mStack.push(&res, 8);
        break;
    }
    case Instruction::DIV_I64: {
        auto lhs = *(int64_t*)mStack.get(8);
        auto rhs = *(int64_t*)mStack.get(0);
        int64_t res = lhs / rhs;
        mStack.pop(16);
        mStack.push(&res, 8);
        break;
    }
    case Instruction::COMPARE_LESS_THAN_I64: {
        auto lhs = *(int64_t*)mStack.get(8);
        auto rhs = *(int64_t*)mStack.get(0);
        mStack.pop(16);
        int64_t res = lhs < rhs;
        mStack.push(&res, BOOL_SIZE);
        break;
    }
    case Instruction::COMPARE_MORE_THAN_I64: {
        auto lhs = *(int64_t*)mStack.get(8);
        auto rhs = *(int64_t*)mStack.get(0);
        mStack.pop(16);
        int64_t res = lhs > rhs;
        mStack.push(&res, BOOL_SIZE);
        break;
    }
    case Instruction::COMPARE_LESS_EQUAL_THAN_I64: {
        auto lhs = *(int64_t*)mStack.get(8);
        auto rhs = *(int64_t*)mStack.get(0);
        mStack.pop(16);
        int64_t res = lhs <= rhs;
        mStack.push(&res, BOOL_SIZE);
        break;
    }
    case Instruction::COMPARE_MORE_EQUAL_THAN_I64: {
        auto lhs = *(int64_t*)mStack.get(8);
        auto rhs = *(int64_t*)mStack.get(0);
        mStack.pop(16);
        int64_t res = lhs >= rhs;
        mStack.push(&res, BOOL_SIZE);
        break;
    }
    case Instruction::LOGICAL_OR: {
#ifdef x86_64_BIT_MODE
        auto lhs = *(bool*)mStack.get(8);
        auto rhs = *(bool*)mStack.get(0);
        mStack.pop(16);
        int64_t res = lhs || rhs;
        mStack.push(&res, 8);
#else
        auto lhs = *(bool*)mStack.get(1);
        auto rhs = *(bool*)mStack.get(0);
        mStack.pop(2);
        bool res = lhs || rhs;
        mStack.push(&res, 1);
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
        int32_t newIp{ -1 };
        auto firstHalfOfParam = *(int32_t*)mStack.get(offset);
        if(firstHalfOfParam % 2 == 0) {
            // it's a lambda function call
            // save old values to stack
            auto* lambdaParams = *(uint8_t**)mStack.get(offset);
            auto lambdaParamsLen = ((int32_t*)lambdaParams)[0];
            newIp = ((int32_t*)lambdaParams)[1];

            // save old values to stack
            *(int32_t*)mStack.get(offset + 4) = mIp + instructionToWidth(Instruction::CALL);
            *(int32_t*)mStack.get(offset) = 0;

            uint8_t buffer[lambdaParamsLen];
            memcpy(buffer, lambdaParams + 16, lambdaParamsLen);
            mStack.push(buffer, lambdaParamsLen);
        } else {
            // it's a default function call or a native function call
            if(firstHalfOfParam == 3) {
                // native

                // save old values
                newIp = mIp + instructionToWidth(Instruction::CALL);
                execNativeFunction(*(int32_t*)mStack.get(offset + 4));
            } else {
                // default
                newIp = *(int32_t*)mStack.get(offset + 4);

                // save old values to stack
                *(int32_t*)mStack.get(offset + 4) = mIp + instructionToWidth(Instruction::CALL);
                *(int32_t*)mStack.get(offset) = 0;
            }
        }

        mIp = newIp;
        incIp = false;
        break;
    }
    case Instruction::RETURN: {
        auto offset = *(int32_t*)&mProgram.code.at(mIp + 1);
        mIp = *(int32_t*)mStack.get(offset + 4);
        mStack.popBelow(offset, 8);
        incIp = false;
        if(mIp == static_cast<int32_t>(mProgram.code.size())) {
            return false;
        }
        break;
    }
    case Instruction::CREATE_LAMBDA: {
        auto functionIpOffset = *(int32_t*)&mProgram.code.at(mIp + 1);
        auto lambdaCapturedTypesId = *(int32_t*)&mProgram.code.at(mIp + 5);
        auto* dataOnHeap = (uint8_t*)mGC.alloc(functionIpOffset + 16);
        // store length of buffer & ip of the function on the stack
        ((int32_t*)dataOnHeap)[0] = functionIpOffset;
        ((int32_t*)dataOnHeap)[1] = *(int32_t*)mStack.get(functionIpOffset);
        ((int32_t*)dataOnHeap)[2] = lambdaCapturedTypesId;
        ((int32_t*)dataOnHeap)[3] = 0;
        memcpy(dataOnHeap + 16, mStack.get(0), functionIpOffset);
        mStack.pop(functionIpOffset + 8);
        mStack.push(&dataOnHeap, 8);
        break;
    }
    case Instruction::CREATE_STRUCT: {
        auto sizeOfData = *(int32_t*)&mProgram.code.at(mIp + 1);
        auto* dataOnHeap = (uint8_t*)mGC.alloc(sizeOfData);
        memcpy(dataOnHeap, mStack.get(0), sizeOfData);
        mStack.pop(sizeOfData);
        mStack.push(&dataOnHeap, 8);
        break;
    }
    case Instruction::CREATE_LIST: {
        auto elementSize = *(int32_t*)&mProgram.code.at(mIp + 1);
        auto elementCount = *(int32_t*)&mProgram.code.at(mIp + 5);
        uint8_t* firstPtr = nullptr;
        uint8_t* ptrToPreviousElement = nullptr;
        for(int i = 0; i < elementCount; ++i) {
            int32_t elementOffset = (elementCount - i - 1) * elementSize;
            auto* dataOnHeap = (uint8_t*)mGC.alloc(elementSize + 8);
            if(firstPtr == nullptr)
                firstPtr = dataOnHeap;

            memcpy(dataOnHeap + 8, mStack.get(elementOffset), elementSize);
            if(ptrToPreviousElement) {
                memcpy(ptrToPreviousElement, &dataOnHeap, 8);
            }
            ptrToPreviousElement = dataOnHeap;
        }
        // let the last element point to nullptr
        if(ptrToPreviousElement)
            memset(ptrToPreviousElement, 0, 8);
        mStack.pop(elementSize * elementCount);
        mStack.push(&firstPtr, 8);
        break;
    }
    case Instruction::LIST_GET_TAIL: {
        if(*(void**)mStack.get(0) != nullptr) {
            *(uint8_t**)mStack.get(0) = **(uint8_t***)mStack.get(0);
        }
        break;
    }
    case Instruction::LOAD_FROM_PTR: {
        auto size = *(int32_t*)&mProgram.code.at(mIp + 1);
        auto offset = *(int32_t*)&mProgram.code.at(mIp + 5);
        auto ptr = *(uint8_t**)mStack.get(0);
        if(ptr == nullptr) {
            // TODO throw some kind of language-internal exception
            throw std::runtime_error{ "Trying to access :head of empty list" };
        }
        mStack.pop(8);
        char buffer[size];
        memcpy(buffer, ptr + offset, size);
        mStack.push(buffer, size);
        break;
    }
    case Instruction::COMPARE_COMPLEX_EQUALITY: {
        auto datatypeIndex = *(int32_t*)&mProgram.code.at(mIp + 1);
        auto& datatype = mProgram.auxiliaryDatatypes.at(datatypeIndex);
        auto datatypeSize = datatype.getSizeOnStack();
        std::function<bool(const Datatype&, uint8_t*, uint8_t*)> isEqual = [&](const Datatype& type, uint8_t* a, uint8_t* b) -> bool {
            switch(type.getCategory()) {
            case DatatypeCategory::list: {
                uint8_t* lhsPtr = *(uint8_t**)a;
                uint8_t* rhsPtr = *(uint8_t**)b;
                while(true) {
                    if(!lhsPtr && !rhsPtr) {
                        return true;
                    }
                    if(!lhsPtr && rhsPtr) {
                        return false;
                    }
                    if(lhsPtr && !rhsPtr) {
                        return false;
                    }
                    if(!isEqual(type.getListContainedType(), lhsPtr + 8, rhsPtr + 8)) {
                        return false;
                    }
                    lhsPtr = *(uint8_t**)lhsPtr;
                    rhsPtr = *(uint8_t**)rhsPtr;
                }
            }
            case DatatypeCategory::i32:
                return *(int32_t*)a == *(int32_t*)b;
            }
            todo();
        };
        int64_t result = isEqual(datatype, (uint8_t*)mStack.get(0), (uint8_t*)mStack.get(datatypeSize));
        mStack.pop(datatypeSize * 2);
        mStack.push(&result, BOOL_SIZE);
        break;
    }
    case Instruction::LIST_PREPEND: {
        auto datatypeLength = *(int32_t*)&mProgram.code.at(mIp + 1);
        uint8_t* allocation = mGC.alloc(datatypeLength + 8);
        memcpy(allocation, mStack.get(0), 8);
        memcpy(allocation + 8, mStack.get(8), datatypeLength);
        mStack.pop(datatypeLength + 8);
        mStack.push(&allocation, 8);
        break;
    }
    case Instruction::IS_LIST_EMPTY: {
        void* list = *(void**)mStack.get(0);
        int64_t result;
        if(list == nullptr) {
            result = 1;
        } else {
            result = 0;
        }
        mStack.pop(8);
        mStack.push(&result, BOOL_SIZE);
        break;
    }
    case Instruction::RUN_GC: {
        mGC.requestCollection();
        break;
    }
    default:
        fprintf(stderr, "Unhandled instruction %i: %s\n", static_cast<int>(ins), instructionToString(ins));
        assert(false);
    }
    mIp += instructionToWidth(ins) * incIp;
    return true;
}
ExternalVMValue VM::run(const std::string& functionName, const std::vector<ExternalVMValue>& params) {
    std::vector<uint8_t> stack(8);
    auto returnValueIP = static_cast<uint32_t>(mProgram.code.size());
    //memset(stack.data(), 0, 8);
    memcpy(stack.data() + 4, &returnValueIP, 4);
    memcpy(stack.data(), &returnValueIP, 4);
    for(auto& param : params) {
        auto stackedValue = param.toStackValue(*this);
        stack.insert(stack.begin(), stackedValue.cbegin(), stackedValue.cend());
    }
    return run(functionName, stack);
}
const Stack& VM::getStack() const {
    return mStack;
}
std::string VM::dumpVariablesOnStack() {
    std::string ret;
    generateStacktrace([&](const uint8_t* ptr, const Datatype& type, const std::string& name){
            ret += " " + name + ": " + ExternalVMValue::wrapFromPtr(type, *this, ptr).dump() + "\n";
        }, [&](const std::string& functionName) {
            ret += functionName + "\n";
        });
    return ret;
}
void VM::generateStacktrace(const std::function<void(const uint8_t* ptr, const Datatype&, const std::string& name)>& variableCallback, const std::function<void(const std::string&)>& functionCallback) const {
    // FIXME: This function isn't quite correct
    int32_t ip = mIp;
    int32_t offsetFromTop = 0;
    while(true) {
        //assert(offsetFromTop >= 0);
        const Program::Function* currentFunction = nullptr;
        for(auto& func : mProgram.functions) {
            if(ip >= func.offset && ip < func.offset + func.len) {
                currentFunction = &func;
                break;
            }
        }
        if(!currentFunction) {
            return;
        }
        if(functionCallback)
            functionCallback(currentFunction->name);
        const auto virtualStackSize = currentFunction->stackSizePerIp.at(ip);
        auto stackInfo = currentFunction->stackInformation->getBestNodeForIp(ip);
        assert(stackInfo);
        const bool isAtReturn = static_cast<Instruction>(mProgram.code.at(ip)) == Instruction::RETURN;

        int32_t nextFunctionOffset = offsetFromTop + virtualStackSize;
        bool afterPop = false;
        while(stackInfo) {
            if(stackInfo->isAtPopInstruction()) {
                afterPop = true;
            }
            auto& variable = stackInfo->getVarEntry();
            if(variable) {
                // after we hit a pop, we don't dump any variables on the stack anymore as they might have been popped of
                if(!afterPop) {
                    if(variableCallback)
                        variableCallback(mStack.getTopPtr() + virtualStackSize - stackInfo->getStackSize() + offsetFromTop, variable->datatype, variable->name);
                }
                // but we still need to account for the size of the parameters on the stack, as those are more related to the previous stackframe
                if(variable->storageType == StorageType::Parameter) {
                    nextFunctionOffset -= variable->datatype.getSizeOnStack();
                }
            }

            if(stackInfo->getPrevSibling()) {
                stackInfo = stackInfo->getPrevSibling();
            } else {
                afterPop = false;
                stackInfo = stackInfo->getParent();
            }
        }
        int32_t prevIp;
        memcpy(&prevIp, mStack.get(offsetFromTop + virtualStackSize + 4), 4);
        if(prevIp == mProgram.code.size()) {
            break;
        }
        if(nextFunctionOffset < 0) {
            // FIXME this shouldn't happen, but it does
            break;
        }
        ip = prevIp - instructionToWidth(Instruction::CALL);
        offsetFromTop = nextFunctionOffset;
    }
}
void VM::execNativeFunction(int32_t nativeFuncId) {
    auto& nativeFunc = mProgram.nativeFunctions.at(nativeFuncId);
    const auto& functionTypeInfo = nativeFunc.functionType.getFunctionTypeInfo();
    auto returnTypeSize = functionTypeInfo.first.getSizeOnStack();
    std::vector<ExternalVMValue> params;
    params.reserve(functionTypeInfo.second.size());
    size_t sizeOfParams = 0;
    for(auto& paramType : functionTypeInfo.second) {
        params.emplace_back(ExternalVMValue::wrapStackedValue(paramType, *this, sizeOfParams));
        sizeOfParams += paramType.getSizeOnStack();
    }
    mStack.pop(sizeOfParams);
    std::reverse(params.begin(), params.end());
    auto returnValue = nativeFunc.callback(*this, params);
    assert(returnValue.getDatatype() == functionTypeInfo.first);
    auto returnValueBytes = returnValue.toStackValue(*this);
    mStack.push(returnValueBytes.data(), returnValueBytes.size());
    mStack.popBelow(returnTypeSize, 8);
}
int32_t VM::getIp() const {
    return mIp;
}
const Program& VM::getProgram() const {
    return mProgram;
}
VM::~VM() = default;
void Stack::push(const std::vector<uint8_t>& data) {
    push(data.data(), data.size());
}
void Stack::push(const void* data, size_t len) {
    ensureSpace(len);
    mDataTop -= len;
    memcpy(mDataTop, data, len);
}
void Stack::repush(size_t offset, size_t len) {
    assert(mDataStart < mDataTop - len - offset);
    ensureSpace(len);
    mDataTop -= len;
    memcpy(mDataTop, mDataTop + len + offset, len);
}
void* Stack::get(size_t offset) {
    return mDataTop + offset;
}
const void* Stack::get(size_t offset) const {
    return mDataTop + offset;
}
void Stack::popBelow(size_t offset, size_t len) {
    assert(getSize() >= offset + len);
    memmove(mDataTop + len, mDataTop, offset);
    mDataTop += len;
}
void Stack::pop(size_t len) {
    mDataTop += len;
}
std::string Stack::dump() {
    std::string ret;
    size_t i = 0;
    for(uint8_t* ptr = mDataTop; ptr < mDataEnd; ++ptr) {
        uint8_t val = *ptr;
        ret += std::to_string(val) + " ";
        ++i;
        if(i > 0 && i % 4 == 0) {
            ret += "\n";
        }
    }
    return ret;
}
Stack::Stack() {
    mDataReserved = 1024 * 1024 * 80;
    mDataStart = (uint8_t*)malloc(mDataReserved);
    mDataEnd = mDataStart + mDataReserved;
    mDataTop = mDataEnd;
}
Stack::~Stack() {
    free(mDataStart);
    mDataStart = nullptr;
    mDataEnd = nullptr;
    mDataTop = nullptr;
}
void Stack::ensureSpace(size_t additionalLen) {
    if(mDataReserved <= getSize() + additionalLen) {
        mDataReserved *= 2;
        mDataStart = (uint8_t*)realloc(mDataStart, mDataReserved);
        mDataEnd = mDataStart + mDataReserved;
        mDataTop = mDataEnd;
    }
}
uint8_t* Stack::getBasePtr() {
    return mDataStart;
}
const uint8_t* Stack::getBasePtr() const {
    return mDataStart;
}
size_t Stack::getSize() const {
    return mDataEnd - mDataTop;
}
void Stack::setSize(size_t val) {
    assert(val <= mDataReserved);
    mDataTop = mDataEnd - val;
}
void Stack::clear() {
    mDataTop = mDataEnd;
}
uint8_t* Stack::getTopPtr() {
    return mDataTop;
}
const uint8_t* Stack::getTopPtr() const {
    return mDataTop;
}

}