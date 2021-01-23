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
                Declaration{ .astNode = decl.get(), .type = decl->getDatatype() });
            mDeclarationNodeToModuleName.emplace(decl.get(), module->getModuleName());
        }
    }
    // FIXME 'compile' (declare) structs in between these two steps
    // compile all normal functions
    for(auto& module : mRoots) {
        for(auto& decl : module->getDeclarations()) {
            if(!decl->hasTemplateParameters() && std::string_view{ decl->getClassName() } == "FunctionDeclarationNode") {
                mCurrentModuleName = module->getModuleName();
                decl->compile(*this);
            }
        }
    }
    // compile all template functions
    while(!mTemplateFunctionsToInstantiate.empty()) {
        auto function = mTemplateFunctionsToInstantiate.front().function;
        mTemplateReplacementMap = createTemplateParamMap(
            function->getIdentifier()->getTemplateParameters(),
            mTemplateFunctionsToInstantiate.front().templateParameters);
        mCurrentModuleName = mDeclarationNodeToModuleName.at(function);
        function->compile(*this);
        mTemplateReplacementMap.reset();
    }
    // insert all the function locations in the code
    for(auto& label: mLabelsToInsertFunctionIds) {
        for(auto& function: mProgram.functions) {
            if(function.second.origin == label.function) {
                auto ptr = labelToPtr(label.label);
                *(reinterpret_cast<Instruction*>(ptr)) = Instruction::PUSH_8;
                *(reinterpret_cast<int32_t*>(ptr + 1)) = function.second.offset;
                *(reinterpret_cast<int32_t*>(ptr + 5)) = 0;

            }
        }
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
    for(auto& param : function.getParameters()) {
        mStackSize += param.type.getSizeOnStack();
        mStackFrames.top().variables.emplace(param.name->getName(), VariableOnStack{ .offsetFromBottom = mStackSize, .type = param.type });
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
    mStackSize = 0;

    // save the region of the function in the program object to allow locating it
    mProgram.functions[fullFunctionName].type = function.getDatatype();
    mProgram.functions[fullFunctionName].offset = start;
    mProgram.functions[fullFunctionName].len = mProgram.code.size() - start;
    mProgram.functions[fullFunctionName].origin = &function;
}
Datatype Compiler::compileScope(const ScopeNode& scope) {
    auto& expressions = scope.getExpressions();
    if(expressions.empty()) {
        return Datatype::createEmptyTuple();
    }

    pushStackFrame();
    Datatype lastType;
    for(auto& expr : scope.getExpressions()) {
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
int32_t Compiler::addLabel(int32_t len) {
    mProgram.code.resize(mProgram.code.size() + len);
    return mProgram.code.size() - len;
}
uint8_t* Compiler::labelToPtr(int32_t label) {
    return &mProgram.code.at(label);
}
Datatype Compiler::compileLiteralI32(int32_t value) {
#ifdef x86_64_BIT_MODE
    mStackSize += 8;
    addInstructions(Instruction::PUSH_8, value, 0);
#else
    mStackSize += 4;
    addInstructions(Instruction::PUSH_4, value);
#endif
    return Datatype(DatatypeCategory::i32);
}
Datatype Compiler::compileBinaryExpression(const BinaryExpressionNode& binaryExpression) {
    auto lhsType = binaryExpression.getLeft()->compile(*this);
    auto rhsType = binaryExpression.getRight()->compile(*this);
    if(lhsType != rhsType) {
        binaryExpression.throwException("Left hand type " + lhsType.toString() + " and right hand type " + rhsType.toString() + " don't match");
    }
    switch(lhsType.getCategory()) {
    case DatatypeCategory::i32:
        switch(binaryExpression.getOperator()) {
        case BinaryExpressionNode::BinaryOperator::PLUS:
            addInstructions(Instruction::ADD_I32);
            mStackSize -= getSimpleSize(DatatypeCategory::i32);
            return Datatype{ DatatypeCategory::i32 };

        case BinaryExpressionNode::BinaryOperator::MINUS:
            addInstructions(Instruction::SUB_I32);
            mStackSize -= getSimpleSize(DatatypeCategory::i32);
            return Datatype{ DatatypeCategory::i32 };

        case BinaryExpressionNode::BinaryOperator::COMPARISON_LESS_THAN:
            addInstructions(Instruction::COMPARE_LESS_THAN_I32);
            mStackSize -= getSimpleSize(DatatypeCategory::i32) * 2;
            mStackSize += getSimpleSize(DatatypeCategory::bool_);
            return Datatype{ DatatypeCategory::bool_ };
        }
    }
    assert(false);
}
Datatype Compiler::compileIfExpression(const IfExpressionNode& ifExpression) {
    std::vector<int32_t> jumpToEndLabels;
    std::optional<Datatype> returnType;
    for(auto& child: ifExpression.getChildren()) {
        auto conditionType = child.first->compile(*this);
        if(conditionType.getCategory() != DatatypeCategory::bool_) {
            child.first->throwException("Condition for if-expressions must be of type bool, not " + conditionType.toString());
        }
        auto jumpToNextLabel = addLabel(instructionToWidth(Instruction::JUMP_IF_FALSE));
        mStackSize -= getSimpleSize(DatatypeCategory::bool_);

        auto bodyReturnType = child.second->compile(*this);
        mStackSize -= bodyReturnType.getSizeOnStack();
        if(returnType) {
            if(*returnType != bodyReturnType) {
                ifExpression.throwException("Each branch of an if-expression must return the same value; other branches returned "
                    + returnType->toString() + ", but one returned " + bodyReturnType.toString());
            }
        } else {
            returnType = std::move(bodyReturnType);
        }
        jumpToEndLabels.push_back(addLabel(instructionToWidth(Instruction::JUMP)));

        auto labelPtr = labelToPtr(jumpToNextLabel);
        *(Instruction*)labelPtr = Instruction::JUMP_IF_FALSE;
        *((int32_t*)((uint8_t*)labelPtr + 1)) = mProgram.code.size();
    }
    assert(returnType);

    if(ifExpression.getElseBody()) {
        auto elseBodyReturnType = ifExpression.getElseBody()->compile(*this);
        mStackSize -= elseBodyReturnType.getSizeOnStack();
        if(elseBodyReturnType != *returnType) {
            ifExpression.throwException("Each branch of an if-expression must return the same value; other branches returned "
                                            + returnType->toString() + ", but the else branch returned " + elseBodyReturnType.toString());
        }
    } else {
        if(returnType != Datatype::createEmptyTuple()) {
            ifExpression.throwException("If an if-expression has no else branch, each other branch must return an empty tuple, but they return " + returnType->toString());
        }
        assert(Datatype::createEmptyTuple().getSizeOnStack() == 0);
    }

    for(auto& label : jumpToEndLabels) {
        auto labelPtr = labelToPtr(label);
        *(Instruction*)labelPtr = Instruction::JUMP;
        *((int32_t*)((uint8_t*)labelPtr + 1)) = mProgram.code.size();
    }
    mStackSize += returnType->getSizeOnStack();

    return *returnType;
}
Datatype Compiler::compileIdentifierLoad(const IdentifierNode& identifier) {
    auto stackCpy = mStackFrames;

    // try looking through the stack
    while(!stackCpy.empty()) {
        auto& stackFrame = stackCpy.top();
        auto maybeVariable = stackFrame.variables.find(identifier.getName());
        if(maybeVariable != stackFrame.variables.end()) {
            addInstructions(Instruction::REPUSH_FROM_N, maybeVariable->second.type.getSizeOnStack(), mStackSize - maybeVariable->second.offsetFromBottom);
            mStackSize += maybeVariable->second.type.getSizeOnStack();
            return maybeVariable->second.type;
        }
        stackCpy.pop();
    }

    // try looking through other functions
    std::string fullName;
    if(identifier.getNameSplit().size() > 1) {
        fullName = identifier.getName();
    } else {
        fullName = mCurrentModuleName + "." + identifier.getName();
    }
    auto maybeDeclaration = mDeclarations.find(fullName);
    if(maybeDeclaration == mDeclarations.end()) {
        identifier.throwException("Identifier not found");
    }
    auto declarationAsFunction = dynamic_cast<FunctionDeclarationNode*>(maybeDeclaration->second.astNode);
    if(!declarationAsFunction) {
        identifier.throwException("Identifier is not a function");
    }
    mLabelsToInsertFunctionIds.emplace_back(FunctionIdInCodeToInsert{.label = addLabel(instructionToWidth(Instruction::PUSH_8)), .function = declarationAsFunction});
    mStackSize += 8;
    return maybeDeclaration->second.type;
}
}