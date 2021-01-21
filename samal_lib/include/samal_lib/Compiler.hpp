#pragma once
#include "Forward.hpp"
#include "Program.hpp"
#include <queue>
#include <stack>
#include "Instruction.hpp"

namespace samal {

class Compiler {
public:
    explicit Compiler(std::vector<up<ModuleRootNode>>& roots);
    Program compile();

    void compileFunction(const FunctionDeclarationNode& function);
    Datatype compileScope(const ScopeNode& scope);
    Datatype compileLiteralI32(int32_t value);
private:
    Program mProgram;
    std::vector<up<ModuleRootNode>>& mRoots;

    void pushStackFrame();
    void popStackFrame(const Datatype& frameReturnType);

    void addInstructions(Instruction insn);
    void addInstructions(Instruction insn, int32_t param);
    void addInstructions(Instruction insn, int32_t param1, int32_t param2);
    void addInstructionOneByteParam(Instruction insn, int8_t param);

    struct TemplateFunctionToInstantiate {
        FunctionDeclarationNode* function{ nullptr };
        // this is e.g. <i32, i64, (i32, bool)>
        std::vector<Datatype> templateParameters;
    };
    std::queue<TemplateFunctionToInstantiate> mTemplateFunctionsToInstantiate;
    struct Declaration {
        DeclarationNode *astNode { nullptr };
        Datatype type;
    };
    std::unordered_map<std::string, Declaration> mDeclarations;

    std::optional<std::map<std::string, Datatype>> mTemplateReplacementMap;

    struct VariableOnStack {
        int32_t offsetFromBottom { 0 };
        Datatype type;
    };
    struct StackFrame {
        std::map<std::string, VariableOnStack> variables;
        int32_t stackFrameSize{ 0 };
    };
    std::stack<StackFrame, std::vector<StackFrame>> mStackFrames;

    int32_t mStackSize { 0 };
};

}