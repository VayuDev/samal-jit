#pragma once
#include "AST.hpp"
#include "Forward.hpp"
#include "Instruction.hpp"
#include "Program.hpp"
#include "Util.hpp"
#include <map>
#include <stack>
#include <vector>

namespace samal {

class ScopeDuration final {
public:
    explicit ScopeDuration(Compiler& compiler, const Datatype& returnType);
    ~ScopeDuration();

private:
    Compiler& mCompiler;
    size_t mReturnTypeSize;
};

class Compiler {
public:
    Compiler();
    Program compile(std::vector<up<ModuleRootNode>>& modules, const std::map<const IdentifierNode*, TemplateInstantiationInfo>& templateInfo);
    void assignToVariable(const up<IdentifierNode>& identifier);
    [[nodiscard]] void compileFunction(const up<IdentifierNode>& identifier, const up<ParameterListNode>& params, std::function<void(Compiler&)> compileBodyCallback);
    [[nodiscard]] ScopeDuration enterScope(const Datatype& returnType);

    size_t addLabel(size_t len);
    void* getLabelPtr(size_t label);
    size_t getCurrentLocation();
    void changeStackSize(ssize_t diff);

    inline void pushPrimitiveLiteralI32(int32_t param) {
#ifdef x86_64_BIT_MODE
        addInstructions(Instruction::PUSH_8, param, 0);
        mStackSize += 8;
#else
        addInstructions(Instruction::PUSH_4, param);
        mStackSize += 4;
#endif
    }
    inline void pushPrimitiveLiteralBool(bool param) {
#ifdef x86_64_BIT_MODE
        addInstructions(Instruction::PUSH_8, param, 0);
        mStackSize += 8;
#else
        addInstructionOneByteParam(Instruction::PUSH_1, param);
        mStackSize += 1;
#endif
    }
    inline void pushPrimitiveLiteralI64(int64_t param) {
        addInstructions(Instruction::PUSH_8, param, param >> 32);
        mStackSize += 8;
    }
    void loadVariableToStack(const IdentifierNode& identifier);
    void popUnusedValueAtEndOfScope(const Datatype& type);
    void binaryOperation(const Datatype& inputTypes, BinaryExpressionNode::BinaryOperator op);
    void performFunctionCall(size_t sizeOfArguments, size_t sizeOfReturnValue);
    void setVariableLocation(const up<IdentifierNode>& identifier);
    void accessTupleElement(const Datatype& tupleType, uint32_t index);
    void moveToTop(size_t len, size_t offset);

private:
    void addInstructions(Instruction insn);
    void addInstructionOneByteParam(Instruction insn, int8_t param);
    void addInstructions(Instruction insn, int32_t param);
    void addInstructions(Instruction insn, int32_t param1, int32_t param2);
    struct VariableInfoOnStack {
        size_t offsetFromTop;
        size_t sizeOnStack;
    };

    struct StackFrame {
        std::map<int32_t, VariableInfoOnStack> variables;
        size_t bytesToPopOnExit{ 0 };
    };

    std::optional<Program> mProgram;
    std::stack<StackFrame> mStackFrames;
    int mStackSize{ 0 };
    // maps function ids to their offset in the code
    std::map<IdentifierId, int32_t> mFunctions;
    // a list of places in the code where the function id was inserted,
    // but the actual ip of the function is required. After the general compilation pass,
    // we have to go through this list and replace for each entry in this array
    // the id at its position with the actual ip of the function it refers to.
    std::vector<size_t> mFunctionIdsInCode;
    const std::map<const IdentifierNode*, TemplateInstantiationInfo>* mTemplateInfo { nullptr };
    std::optional<std::map<std::string, Datatype>> mCurrentTemplateReplacementsMap;

    Datatype performTemplateReplacement(const Datatype& source);

    friend class FunctionDuration;
    friend class ScopeDuration;
};

}