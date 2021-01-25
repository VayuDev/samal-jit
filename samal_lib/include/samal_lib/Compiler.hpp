#pragma once
#include "Forward.hpp"
#include "Instruction.hpp"
#include "Program.hpp"
#include <queue>
#include <stack>

namespace samal {

class Compiler {
public:
    explicit Compiler(std::vector<up<ModuleRootNode>>& roots);
    Program compile();

    void compileFunction(const FunctionDeclarationNode& function);
    Datatype compileScope(const ScopeNode& scope);
    Datatype compileLiteralI32(int32_t value);
    Datatype compileLiteralI64(int64_t value);
    Datatype compileLiteralBool(bool value);
    Datatype compileBinaryExpression(const BinaryExpressionNode&);
    Datatype compileIfExpression(const IfExpressionNode&);
    Datatype compileIdentifierLoad(const IdentifierNode&);
    Datatype compileFunctionCall(const FunctionCallExpressionNode&);
    Datatype compileChainedFunctionCall(const FunctionChainExpressionNode&);
    Datatype compileTupleCreationExpression(const TupleCreationNode&);
    Datatype compileTupleAccessExpression(const TupleAccessExpressionNode&);
    Datatype compileAssignmentExpression(const AssignmentExpression&);

private:
    Program mProgram;
    std::vector<up<ModuleRootNode>>& mRoots;

    void pushStackFrame();
    void popStackFrame(const Datatype& frameReturnType);

    void addInstructions(Instruction insn);
    void addInstructions(Instruction insn, int32_t param);
    void addInstructions(Instruction insn, int32_t param1, int32_t param2);
    void addInstructionOneByteParam(Instruction insn, int8_t param);

    int32_t addLabel(int32_t len);
    uint8_t* labelToPtr(int32_t label);
    struct TemplateFunctionToInstantiate {
        FunctionDeclarationNode* function{ nullptr };
        // this is e.g. <i32, i64, (i32, bool)>
        std::map<std::string, Datatype> replacementMap;
    };
    std::vector<TemplateFunctionToInstantiate> mTemplateFunctionsToInstantiate;
    struct Declaration {
        DeclarationNode* astNode{ nullptr };
        Datatype type;
    };
    std::unordered_map<std::string, Declaration> mDeclarations;
    std::unordered_map<DeclarationNode*, std::string> mDeclarationNodeToModuleName;
    std::string mCurrentModuleName;

    std::map<std::string, Datatype> mTemplateReplacementMap;

    struct VariableOnStack {
        int32_t offsetFromBottom{ 0 };
        Datatype type;
    };
    struct StackFrame {
        std::map<std::string, VariableOnStack> variables;
        int32_t stackFrameSize{ 0 };
    };
    std::stack<StackFrame, std::vector<StackFrame>> mStackFrames;

    int32_t mStackSize{ 0 };

    struct FunctionIdInCodeToInsert {
        int32_t label{ -1 };
        FunctionDeclarationNode* function{ nullptr };
        std::map<std::string, Datatype> templateParameters;
    };
    std::vector<FunctionIdInCodeToInsert> mLabelsToInsertFunctionIds;
};

}