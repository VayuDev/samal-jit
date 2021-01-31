#include "samal_lib/VM.hpp"
#include "samal_lib/ExternalVMValue.hpp"
#include "samal_lib/Instruction.hpp"
#include "samal_lib/StackInformationTree.hpp"
#include "samal_lib/Util.hpp"
#include <cassert>
#include <cstdlib>
#include <cstring>

namespace samal {

#ifdef SAMAL_ENABLE_JIT
struct JitReturn {
    int32_t ip;        // lower 4 bytes of rax
    int32_t stackSize; // upper 4 bytes of rax
    int32_t nativeFunctionToCall; // lower 4 bytes of rdx
};

[[maybe_unused]] static constexpr bool isJittable(Instruction i) {
    switch(i) {
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
    : Xbyak::CodeGenerator(4096 * 4) {
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
            case Instruction::PUSH_8:
                if(nextInstruction() == Instruction::SUB_I32) {
                    pop(rbx);
                    sub(rbx, *(uint64_t*)&instructions.at(i + 1));
                    push(rbx);
                    add(ip, instructionToWidth(ins) + nextInstructionWidth());
                    i += instructionToWidth(ins) + nextInstructionWidth();
                    shouldAutoIncrement = false;
                } else if(nextInstruction() == Instruction::ADD_I32) {
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
                add(ebx, eax);
                push(rbx);
                break;
            case Instruction::SUB_I32:
                pop(rax);
                pop(rbx);
                sub(ebx, eax);
                push(rbx);
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
                pop(rbx);
                add(rbx, rax);
                push(rbx);
                break;
            case Instruction::SUB_I64:
                pop(rax);
                pop(rbx);
                sub(rbx, rax);
                push(rbx);
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
                // TODO replace movsq()-code with simple mov code, similar to REPUSH_FROM_N

                //    init copy
                assert(popOffset % 8 == 0);
                mov(rsi, rsp);
                add(rsi, popOffset - 8);

                mov(rdi, rsp);
                add(rdi, popOffset + popLen - 8);

                std();
                //    copy
                for(int j = 0; j < popOffset / 8; ++j) {
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
                    add(rsi, 8);
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
                mov(qword[rax], ip);

                mov(ip, rbx);
                jumpWithIp();
                break;
            }
            case Instruction::RETURN: {
                int32_t returnInfoOffset = *(uint32_t*)&instructions.at(i + 1);
                mov(ip, qword[rsp + returnInfoOffset]);

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
                    for(int j = 0; j < popOffset / 8; ++j) {
                        movsq();
                    }

                    add(rsp, popLen);
                }

                jumpWithIp();
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

        /*L("JumpTable");
        for (auto& label : labels) {
            mov(rax, label.first);
            cmp(rax, ip);
            je(label.second);
        }*/

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

        // do some magic to put but stackSize and ip in the rdx register
        mov(rax, stackSize);
        sal(rax, 32);
        mov(rbx, ip);
        mov(ebx, ebx);
        or_(rax, rbx);

        // put native function id in the rax register
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
    }
};
#else
class JitCode {
};
#endif

VM::VM(Program program)
: mProgram(std::move(program)) {
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
    auto returnType = *function->type.getFunctionTypeInfo().first;
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
            std::reverse((int64_t*)mStack.getBasePtr(), (int64_t*)(mStack.getBasePtr() + mStack.getSize()));
            JitReturn ret = mCompiledCode->getCode<JitReturn (*)(int32_t, uint8_t*, int32_t)>()(mIp, mStack.getBasePtr(), mStack.getSize());
            mStack.setSize(ret.stackSize);
            mIp = ret.ip;
            std::reverse((int64_t*)mStack.getBasePtr(), (int64_t*)(mStack.getBasePtr() + mStack.getSize()));
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
#ifdef _DEBUG
    // dump
    auto varDump = dumpVariablesOnStack();
    printf("Variables: %s", varDump.c_str());
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
    case Instruction::COMPARE_MORE_THAN_I32: {
#ifdef x86_64_BIT_MODE
        auto lhs = *(int32_t*)mStack.get(16);
        auto rhs = *(int32_t*)mStack.get(8);
        mStack.pop(16);
        int64_t res = lhs > rhs;
        mStack.push(&res, 8);
#else
        auto lhs = *(int32_t*)mStack.get(8);
        auto rhs = *(int32_t*)mStack.get(4);
        mStack.pop(8);
        bool res = lhs > rhs;
        mStack.push(&res, 1);
#endif
        break;
    }
    case Instruction::COMPARE_LESS_EQUAL_THAN_I32: {
#ifdef x86_64_BIT_MODE
        auto lhs = *(int32_t*)mStack.get(16);
        auto rhs = *(int32_t*)mStack.get(8);
        mStack.pop(16);
        int64_t res = lhs <= rhs;
        mStack.push(&res, 8);
#else
        auto lhs = *(int32_t*)mStack.get(8);
        auto rhs = *(int32_t*)mStack.get(4);
        mStack.pop(8);
        bool res = lhs <= rhs;
        mStack.push(&res, 1);
#endif
        break;
    }
    case Instruction::COMPARE_MORE_EQUAL_THAN_I32: {
#ifdef x86_64_BIT_MODE
        auto lhs = *(int32_t*)mStack.get(16);
        auto rhs = *(int32_t*)mStack.get(8);
        mStack.pop(16);
        int64_t res = lhs >= rhs;
        mStack.push(&res, 8);
#else
        auto lhs = *(int32_t*)mStack.get(8);
        auto rhs = *(int32_t*)mStack.get(4);
        mStack.pop(8);
        bool res = lhs >= rhs;
        mStack.push(&res, 1);
#endif
        break;
    }
    case Instruction::SUB_I64: {
        auto lhs = *(int64_t*)mStack.get(16);
        auto rhs = *(int64_t*)mStack.get(8);
        int64_t res = lhs - rhs;
        mStack.pop(16);
        mStack.push(&res, 8);
        break;
    }
    case Instruction::ADD_I64: {
        auto lhs = *(int64_t*)mStack.get(16);
        auto rhs = *(int64_t*)mStack.get(8);
        int64_t res = lhs + rhs;
        mStack.pop(16);
        mStack.push(&res, 8);
        break;
    }
    case Instruction::COMPARE_LESS_THAN_I64: {
        auto lhs = *(int64_t*)mStack.get(16);
        auto rhs = *(int64_t*)mStack.get(8);
        mStack.pop(16);
        bool res = lhs < rhs;
        mStack.push(&res, 1);
        break;
    }
    case Instruction::COMPARE_MORE_THAN_I64: {
        auto lhs = *(int64_t*)mStack.get(16);
        auto rhs = *(int64_t*)mStack.get(8);
        mStack.pop(16);
        bool res = lhs > rhs;
        mStack.push(&res, 1);
        break;
    }
    case Instruction::COMPARE_LESS_EQUAL_THAN_I64: {
        auto lhs = *(int64_t*)mStack.get(16);
        auto rhs = *(int64_t*)mStack.get(8);
        mStack.pop(16);
        bool res = lhs <= rhs;
        mStack.push(&res, 1);
        break;
    }
    case Instruction::COMPARE_MORE_EQUAL_THAN_I64: {
        auto lhs = *(int64_t*)mStack.get(16);
        auto rhs = *(int64_t*)mStack.get(8);
        mStack.pop(16);
        bool res = lhs >= rhs;
        mStack.push(&res, 1);
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
        auto firstHalfOfParam = *(int32_t*)mStack.get(offset + 8);
        if(firstHalfOfParam % 2 == 0) {
            // it's a lambda function call
            // save old values to stack
            auto* lambdaParams = *(uint8_t**)mStack.get(offset + 8);
            auto lambdaParamsLen = ((int32_t*)lambdaParams)[0];
            newIp = ((int32_t*)lambdaParams)[1];

            // save old values to stack
            *(int32_t*)mStack.get(offset + 8) = mIp + instructionToWidth(Instruction::CALL);
            *(int32_t*)mStack.get(offset + 4) = 0;

            uint8_t buffer[lambdaParamsLen];
            memcpy(buffer, lambdaParams + 8, lambdaParamsLen);
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
                *(int32_t*)mStack.get(offset + 8) = mIp + instructionToWidth(Instruction::CALL);
                *(int32_t*)mStack.get(offset + 4) = 0;
            }
        }

        mIp = newIp;
        incIp = false;
        break;
    }
    case Instruction::RETURN: {
        auto offset = *(int32_t*)&mProgram.code.at(mIp + 1);
        mIp = *(int32_t*)mStack.get(offset + 8);
        mStack.popBelow(offset, 8);
        incIp = false;
        if(mIp == static_cast<int32_t>(mProgram.code.size())) {
            return false;
        }
        break;
    }
    case Instruction::CREATE_LAMBDA: {
        auto functionIdOffset = *(int32_t*)&mProgram.code.at(mIp + 1);
        auto* dataOnHeap = (uint8_t*)mGC.alloc(functionIdOffset + 8);
        // store length of buffer & ip of the function on the stack
        ((int32_t*)dataOnHeap)[0] = functionIdOffset;
        ((int32_t*)dataOnHeap)[1] = *(int32_t*)mStack.get(functionIdOffset + 8);
        memcpy(dataOnHeap + 8, mStack.get(functionIdOffset), functionIdOffset);
        mStack.pop(functionIdOffset + 8);
        mStack.push(&dataOnHeap, 8);
        break;
    }
    case Instruction::RETURN_FROM_LAMBDA: {
        auto offset = *(int32_t*)&mProgram.code.at(mIp + 1);
        mIp = *(int64_t*)mStack.get(offset + 8);
        mStack.popBelow(offset, 8);
        incIp = false;
        if(mIp == static_cast<int32_t>(mProgram.code.size())) {
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
ExternalVMValue VM::run(const std::string& functionName, const std::vector<ExternalVMValue>& params) {
    std::vector<uint8_t> stack(8);
    auto returnValueIP = static_cast<uint32_t>(mProgram.code.size());
    memcpy(stack.data(), &returnValueIP, 4);
    memset(stack.data() + 4, 0, 4);
    for(auto& param : params) {
        auto stackedValue = param.toStackValue(*this);
        stack.insert(stack.end(), stackedValue.cbegin(), stackedValue.cend());
    }
    return run(functionName, stack);
}
const Stack& VM::getStack() const {
    return mStack;
}
std::string VM::dumpVariablesOnStack() {
    std::string ret;
    Program::Function* currentFunction = nullptr;
    for(auto& func : mProgram.functions) {
        if(mIp >= func.offset && mIp < func.offset + func.len) {
            currentFunction = &func;
            break;
        }
    }
    if(!currentFunction) {
        return ret;
    }
    auto virtualStackSize = currentFunction->stackSizePerIp.at(mIp);
    auto stackInfo = currentFunction->stackInformation->getBestNodeForIp(mIp);
    assert(stackInfo);
    while(stackInfo) {
        if(stackInfo->isAtPopInstruction()) {
            stackInfo = stackInfo->getParent();
            continue;
        }
        auto variable = stackInfo->getVarEntry();
        if(variable) {
            ret += variable->name + ": " + ExternalVMValue::wrapStackedValue(variable->type, *this, virtualStackSize - stackInfo->getStackSize()).dump() + "\n";
        }

        if(stackInfo->getPrevSibling()) {
            stackInfo = stackInfo->getPrevSibling();
        } else {
            stackInfo = stackInfo->getParent();
        }
    }

    return ret;
}
void VM::execNativeFunction(int32_t nativeFuncId) {
    auto& nativeFunc = mProgram.nativeFunctions.at(nativeFuncId);
    auto returnTypeSize = nativeFunc.returnType.getSizeOnStack();
    std::vector<ExternalVMValue> params;
    size_t sizeOfParams = 0;
    for(auto& paramType : nativeFunc.paramTypes) {
        params.emplace_back(ExternalVMValue::wrapStackedValue(paramType, *this, sizeOfParams));
        sizeOfParams += paramType.getSizeOnStack();
    }
    mStack.pop(sizeOfParams);
    std::reverse(params.begin(), params.end());
    auto returnValue = nativeFunc.callback(params);
    assert(returnValue.getDatatype() == nativeFunc.returnType);
    auto returnValueBytes = returnValue.toStackValue(*this);
    mStack.push(returnValueBytes.data(), returnValueBytes.size());
    mStack.popBelow(returnTypeSize, 8);
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
uint8_t* Stack::getBasePtr() {
    return mData;
}
const uint8_t* Stack::getBasePtr() const {
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