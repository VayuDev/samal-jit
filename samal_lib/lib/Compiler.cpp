#include "samal_lib/Compiler.hpp"
#include "samal_lib/AST.hpp"

namespace samal {

Compiler::Compiler(std::vector<up<ModuleRootNode>>& roots)
: mRoots(roots) {
}
Program Compiler::compile() {
    mProgram = {};
    // declare all functions & structs
    for(auto& module : mRoots) {
        for(auto& decl : module->getDeclarations()) {
            mDeclarations.emplace(
                module->getModuleName() + '.' + decl->getIdentifier()->getName(),
                Declaration{.astNode = decl.get(), .type = decl->getDatatype()});
        }
    }
    // FIXME 'compile' (declare) structs in between these two steps
    // compile all normal functions
    for(auto& module : mRoots) {
        for(auto& decl : module->getDeclarations()) {
            if(!decl->hasTemplateParameters() && std::string_view{decl->getClassName()} == "FunctionDeclarationNode") {
                decl->compile(*this);
            }
        }
    }
    // compile all template functions
    while(!mTemplateFunctionsToInstantiate.empty()) {
        mTemplateReplacementMap = createTemplateParamMap(
            mTemplateFunctionsToInstantiate.front().function->getIdentifier()->getTemplateParameters(),
            mTemplateFunctionsToInstantiate.front().templateParameters);
        mTemplateFunctionsToInstantiate.front().function->compile(*this);
        mTemplateReplacementMap.reset();
    }
    return std::move(mProgram);
}
void Compiler::compileFunction(const FunctionDeclarationNode& function) {
    // search for the full function name (this is the name prepended by the module)
    // TODO maybe add reverse list?
    std::string fullFunctionName;
    for(const auto& [name, decl] : mDeclarations) {
        if(decl.astNode == &function) {
            fullFunctionName = name;
        }
    }
    assert(!fullFunctionName.empty());
    auto start = mProgram.code.size();

    pushStackFrame();
    const auto& functionReturnType = function.getReturnType();
    for(auto& param: function.getParameters()) {
        mStackSize += param.type.getSizeOnStack();
        mStackFrames.top().variables.emplace(param.name->getName(), VariableOnStack{.offsetFromBottom = mStackSize, .type = param.type});
    }
    mStackFrames.top().stackFrameSize = mStackSize;
    auto bodyReturnType = function.getBody()->compile(*this);
    if(bodyReturnType != functionReturnType) {
        function.throwException("Function's declared return type " + functionReturnType.toString() + " and actual return type " + bodyReturnType.toString() + " don't match");
    }

    // pop all parameters of the stack
    {
        auto bytesToPop = mStackFrames.top().stackFrameSize;
        if(bytesToPop > 0) {
            addInstructions(Instruction::POP_N_BELOW, bytesToPop, functionReturnType.getSizeOnStack());
            mStackSize -= bytesToPop;
        }
        mStackFrames.pop();
    }
    assert(mStackSize == static_cast<int32_t>(functionReturnType.getSizeOnStack()));
    addInstructions(Instruction::RETURN, functionReturnType.getSizeOnStack());

    // save the region of the function in the program object to allow locating it
    mProgram.functions[fullFunctionName].type = function.getDatatype();
    mProgram.functions[fullFunctionName].offset = start;
    mProgram.functions[fullFunctionName].len = mProgram.code.size() - start;
}
Datatype Compiler::compileScope(const ScopeNode& scope) {
    auto& expressions = scope.getExpressions();
    if(expressions.empty()) {
        return Datatype::createEmptyTuple();
    }

    pushStackFrame();
    Datatype lastType;
    for(auto& expr: scope.getExpressions()) {
        lastType = expr->compile(*this);
        mStackFrames.top().stackFrameSize += lastType.getSizeOnStack();
    }
    popStackFrame(lastType);
    return lastType;
}
void Compiler::pushStackFrame() {
    mStackFrames.push({});
}
void Compiler::popStackFrame(const Datatype& frameReturnType) {
    auto returnTypeSize = frameReturnType.getSizeOnStack();
    auto bytesToPop = mStackFrames.top().stackFrameSize - returnTypeSize;
    if(bytesToPop > 0) {
        addInstructions(Instruction::POP_N_BELOW, bytesToPop, returnTypeSize);
        mStackSize -= bytesToPop;
    }
    mStackFrames.pop();
}

void Compiler::addInstructions(Instruction insn, int32_t param) {
    printf("Adding instruction %s %i\n", instructionToString(insn), param);
    mProgram.code.resize(mProgram.code.size() + 5);
    memcpy(&mProgram.code.at(mProgram.code.size() - 5), &insn, 1);
    memcpy(&mProgram.code.at(mProgram.code.size() - 4), &param, 4);
}
void Compiler::addInstructions(Instruction ins) {
    printf("Adding instruction %s\n", instructionToString(ins));
    mProgram.code.resize(mProgram.code.size() + 1);
    memcpy(&mProgram.code.at(mProgram.code.size() - 1), &ins, 1);
}
void Compiler::addInstructions(Instruction insn, int32_t param1, int32_t param2) {
    printf("Adding instruction %s %i %i\n", instructionToString(insn), param1, param2);
    mProgram.code.resize(mProgram.code.size() + 9);
    memcpy(&mProgram.code.at(mProgram.code.size() - 9), &insn, 1);
    memcpy(&mProgram.code.at(mProgram.code.size() - 8), &param1, 4);
    memcpy(&mProgram.code.at(mProgram.code.size() - 4), &param2, 4);
}
void Compiler::addInstructionOneByteParam(Instruction insn, int8_t param) {
    printf("Adding instruction %s %i\n", instructionToString(insn), (int)param);
    mProgram.code.resize(mProgram.code.size() + 2);
    memcpy(&mProgram.code.at(mProgram.code.size() - 2), &insn, 1);
    memcpy(&mProgram.code.at(mProgram.code.size() - 1), &param, 1);
}
Datatype Compiler::compileLiteralI32(int32_t value) {
#ifdef x86_64_BIT_MODE
    mStackSize += 8;
    addInstructions(Instruction::PUSH_8, value, 0);
    return Datatype(DatatypeCategory::i64);
#else
    mStackSize += 4;
    addInstructions(Instruction::PUSH_4, value);
    return Datatype(DatatypeCategory::i32);
#endif
}
}