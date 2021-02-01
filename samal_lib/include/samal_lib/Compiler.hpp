#pragma once
#include "Forward.hpp"
#include "Instruction.hpp"
#include "Program.hpp"
#include <queue>
#include <stack>
#include <unordered_map>

namespace samal {

class VariableSearcher {
public:
    explicit VariableSearcher(std::vector<const IdentifierNode*>& identifiers);
    void identifierFound(const IdentifierNode& identifier);

private:
    std::vector<const IdentifierNode*>& mIdentifiers;
};

class Compiler final {
public:
    explicit Compiler(std::vector<up<ModuleRootNode>>& roots, std::vector<NativeFunction>&& nativeFunctions);
    ~Compiler();
    Program compile();

    void compileFunction(const FunctionDeclarationNode& function);
    void compileNativeFunction(const NativeFunctionDeclarationNode& function);
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
    Datatype compileLambdaCreationExpression(const LambdaCreationNode&);

private:
    Program mProgram;
    std::vector<up<ModuleRootNode>>& mRoots;

    void pushStackFrame();
    void popStackFrame(const Datatype& frameReturnType);

    void addInstructions(Instruction insn);
    void addInstructions(Instruction insn, int32_t param);
    void addInstructions(Instruction insn, int32_t param1, int32_t param2);
    void addInstructionOneByteParam(Instruction insn, int8_t param);

    void saveVariableLocation(std::string name, Datatype type);
    void saveCurrentStackSizeToDebugInfo();

    enum class IsLambda {
        Yes,
        No
    };
    Program::Function& compileFunctionlikeThing(const std::string& name, const Datatype& returnType, const std::vector<std::pair<std::string, Datatype>>& params, const ScopeNode& body, IsLambda);

    int32_t addLabel(Instruction futureInstruction);
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
        std::string fullFunctionName;
        std::map<std::string, Datatype> templateParameters;
    };
    std::vector<FunctionIdInCodeToInsert> mLabelsToInsertFunctionIds;

    struct LambdaIdInCodeToInsert {
        int32_t label{ -1 };
        const LambdaCreationNode* lambda{ nullptr };
    };
    std::vector<LambdaIdInCodeToInsert> mLabelsToInsertLambdaIds;

    struct LambdaToCompile {
        const LambdaCreationNode* lambda{ nullptr };
        std::vector<std::pair<std::string, Datatype>> copiedParameters;
    };
    std::queue<LambdaToCompile> mLambdasToCompile;

    up<StackInformationTree> mCurrentStackInfoTree;
    StackInformationTree* mCurrentStackInfoTreeNode{ nullptr };
    std::unordered_map<int32_t, int32_t> mIpToStackSize;

    Program::Function& compileLambda(const LambdaToCompile&);
};

}