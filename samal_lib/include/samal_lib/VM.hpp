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

struct VMParameters {
    int32_t functionsCallsPerGCRun = 400000;
    int32_t initialHeapSize = 1024 * 1024;
};

class VM final {
public:
    explicit VM(Program program, VMParameters params = {});
    ~VM();
    ExternalVMValue run(const std::string& functionName, std::vector<uint8_t> initialStack);
    ExternalVMValue run(const std::string& functionName, const std::vector<ExternalVMValue>& params);
    [[nodiscard]] const Stack& getStack() const;
    [[nodiscard]] const Program& getProgram() const;

    void generateStacktrace(const std::function<void(const uint8_t* ptr, const Datatype&, const std::string& name)>& variableCallback, const std::function<void(const std::string&)>& functionCallback) const;
    std::string dumpVariablesOnStack();
    int32_t getIp() const;
    inline uint8_t* alloc(int32_t len) {
        return mGC.alloc(len);
    }

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