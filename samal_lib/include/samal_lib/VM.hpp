#pragma once

#include "Forward.hpp"
#include "GC.hpp"
#include "Program.hpp"
#include "Util.hpp"
#ifdef SAMAL_ENABLE_JIT
#    include <xbyak/xbyak.h>
#endif

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
    const void* get(size_t offset) const;
    std::string dump();
    uint8_t* getBasePtr();
    uint8_t* getTopPtr();
    const uint8_t* getTopPtr() const;
    const uint8_t* getBasePtr() const;
    size_t getSize() const;
    void setSize(size_t);
    void clear();

private:
    void ensureSpace(size_t additionalLen);

    uint8_t* mDataStart;
    uint8_t* mDataEnd;
    uint8_t* mDataTop;
    size_t mDataReserved{ 0 };
};

struct StackTrace {
    struct StackFrame {
        struct Variable {
            std::string name;
            Datatype type;
            const uint8_t* ptr { nullptr };
        };
        std::vector<Variable> variables;
        std::string functionName;
    };
    std::vector<StackFrame> stackFrames;
};

class VM final {
public:
    explicit VM(Program program);
    ~VM();
    ExternalVMValue run(const std::string& functionName, std::vector<uint8_t> initialStack);
    ExternalVMValue run(const std::string& functionName, const std::vector<ExternalVMValue>& params);
    [[nodiscard]] const Stack& getStack() const;
    [[nodiscard]] const Program& getProgram() const;

    StackTrace generateStacktrace() const;
    std::string dumpVariablesOnStack();
    int32_t getIp() const;

private:
    __always_inline bool interpretInstruction();
    void execNativeFunction(int32_t id);

    Stack mStack;
    Program mProgram;
    int32_t mIp = 0;
    up<class JitCode> mCompiledCode;
    GC mGC;
};

}