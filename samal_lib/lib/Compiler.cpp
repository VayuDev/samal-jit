#include "samal_lib/Compiler.hpp"
#include "samal_lib/AST.hpp"
#include "samal_lib/StackInformationTree.hpp"

namespace samal {

Compiler::Compiler(std::vector<up<ModuleRootNode>>& roots, std::vector<NativeFunction>&& nativeFunctions)
: mRoots(roots), mNativeFunctions(std::move(nativeFunctions)) {
}
Compiler::~Compiler() = default;

Program Compiler::compile() {
    mProgram = {};
    // declare all functions & structs
    for(auto& module : mRoots) {
        for(auto& decl : module->getDeclarations()) {
            auto emplaceResult = mDeclarations.emplace(
                module->getModuleName() + '.' + decl->getIdentifier()->getName(),
                Declaration{ .astNode = decl.get(), .type = decl->getDatatype() });
            if(!emplaceResult.second) {
                decl->throwException("Function defined twice!");
            }
            mDeclarationNodeToModuleName.emplace(decl.get(), module->getModuleName());
        }
    }
    // TODO 'compile' (declare) structs in between these two steps

    auto compileLambdaFunctions = [this] {
        while(!mLambdasToCompile.empty()) {
            auto& compiledLambda = compileLambda(mLambdasToCompile.front());
            // find all labels that refer to the lambda we just compiled
            for(auto itr = mLabelsToInsertLambdaIds.begin(); itr != mLabelsToInsertLambdaIds.end();) {
                if(itr->lambda == mLambdasToCompile.front().lambda) {
                    auto ptr = labelToPtr(itr->label);
                    *(reinterpret_cast<Instruction*>(ptr)) = Instruction::PUSH_8;
                    *(reinterpret_cast<int32_t*>(ptr + 1)) = compiledLambda.offset;
                    *(reinterpret_cast<int32_t*>(ptr + 5)) = 0;
                    mLabelsToInsertLambdaIds.erase(itr);
                } else {
                    itr++;
                }
            }
            mLambdasToCompile.pop();
        }
        assert(mLambdasToCompile.empty());
        assert(mLabelsToInsertLambdaIds.empty());
    };
    // compile all normal functions
    for(auto& module : mRoots) {
        mCurrentModuleName = module->getModuleName();
        for(auto& decl : module->getDeclarations()) {
            if(!decl->hasTemplateParameters() && std::string_view{ decl->getClassName() } == "FunctionDeclarationNode") {
                decl->compile(*this);
                compileLambdaFunctions();
            }
        }
    }
    // compile all template functions
    while(!mTemplateFunctionsToInstantiate.empty()) {
        auto function = mTemplateFunctionsToInstantiate.front().function;
        mTemplateReplacementMap = mTemplateFunctionsToInstantiate.front().replacementMap;
        mCurrentModuleName = mDeclarationNodeToModuleName.at(function);
        function->compile(*this);
        mTemplateFunctionsToInstantiate.erase(mTemplateFunctionsToInstantiate.begin());
        compileLambdaFunctions();
        mTemplateReplacementMap.clear();
    }
    // insert all the function locations in the code
    for(auto& label : mLabelsToInsertFunctionIds) {
        bool found = false;
        for(auto& function : mProgram.functions) {
            if(function.name == label.fullFunctionName && function.templateParameters == label.templateParameters) {
                auto ptr = labelToPtr(label.label);
                *(reinterpret_cast<Instruction*>(ptr)) = Instruction::PUSH_8;
                // the makes it clear to the vm that this is a function, not a lambda
                *(reinterpret_cast<int32_t*>(ptr + 1)) = 1;
                *(reinterpret_cast<int32_t*>(ptr + 5)) = function.offset;
                found = true;
                break;
            }
        }
        assert(found);
    }
    mProgram.nativeFunctions = mNativeFunctions;
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
    std::vector<std::pair<std::string, Datatype>> functionParams;
    for(auto& param : function.getParameters()) {
        functionParams.emplace_back(param.name->getName(), param.type);
    }
    compileFunctionlikeThing(fullFunctionName, function.getReturnType(), std::move(functionParams), *function.getBody(), IsLambda::No);
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
    if(mCurrentStackInfoTreeNode) {
        mCurrentStackInfoTreeNode = mCurrentStackInfoTreeNode->addChild(std::make_unique<StackInformationTree>(mProgram.code.size(), mStackSize, StackInformationTree::IsAtPopInstruction::No));
    }
}
void Compiler::popStackFrame(const Datatype& frameReturnType) {
    auto returnTypeSize = frameReturnType.getSizeOnStack();
    auto bytesToPop = mStackFrames.top().stackFrameSize - returnTypeSize;
    if(bytesToPop > 0) {
        addInstructions(Instruction::POP_N_BELOW, bytesToPop, returnTypeSize);
        mStackSize -= bytesToPop;
    }
    mStackFrames.pop();
    mCurrentStackInfoTreeNode->addChild(std::make_unique<StackInformationTree>(mProgram.code.size(), mStackSize, StackInformationTree::IsAtPopInstruction::Yes));
    mCurrentStackInfoTreeNode = mCurrentStackInfoTreeNode->getParent();
}

void Compiler::addInstructions(Instruction insn, int32_t param) {
    saveCurrentStackSizeToDebugInfo();
    printf("Adding instruction %s %i\n", instructionToString(insn), param);
    mProgram.code.resize(mProgram.code.size() + 5);
    memcpy(&mProgram.code.at(mProgram.code.size() - 5), &insn, 1);
    memcpy(&mProgram.code.at(mProgram.code.size() - 4), &param, 4);
}
void Compiler::addInstructions(Instruction ins) {
    saveCurrentStackSizeToDebugInfo();
    printf("Adding instruction %s\n", instructionToString(ins));
    mProgram.code.resize(mProgram.code.size() + 1);
    memcpy(&mProgram.code.at(mProgram.code.size() - 1), &ins, 1);
}
void Compiler::addInstructions(Instruction insn, int32_t param1, int32_t param2) {
    saveCurrentStackSizeToDebugInfo();
    printf("Adding instruction %s %i %i\n", instructionToString(insn), param1, param2);
    mProgram.code.resize(mProgram.code.size() + 9);
    memcpy(&mProgram.code.at(mProgram.code.size() - 9), &insn, 1);
    memcpy(&mProgram.code.at(mProgram.code.size() - 8), &param1, 4);
    memcpy(&mProgram.code.at(mProgram.code.size() - 4), &param2, 4);
}
void Compiler::addInstructionOneByteParam(Instruction insn, int8_t param) {
    saveCurrentStackSizeToDebugInfo();
    printf("Adding instruction %s %i\n", instructionToString(insn), (int)param);
    mProgram.code.resize(mProgram.code.size() + 2);
    memcpy(&mProgram.code.at(mProgram.code.size() - 2), &insn, 1);
    memcpy(&mProgram.code.at(mProgram.code.size() - 1), &param, 1);
}
int32_t Compiler::addLabel(Instruction insn) {
    saveCurrentStackSizeToDebugInfo();
    auto len = instructionToWidth(insn);
    mProgram.code.resize(mProgram.code.size() + len);
    return mProgram.code.size() - len;
}
uint8_t* Compiler::labelToPtr(int32_t label) {
    return &mProgram.code.at(label);
}
Datatype Compiler::compileLiteralI32(int32_t value) {
#ifdef x86_64_BIT_MODE
    addInstructions(Instruction::PUSH_8, value, 0);
    mStackSize += 8;
#else
    addInstructions(Instruction::PUSH_4, value);
    mStackSize += 4;
#endif
    return Datatype(DatatypeCategory::i32);
}
Datatype Compiler::compileLiteralI64(int64_t value) {
    addInstructions(Instruction::PUSH_8, value, value >> 32);
    mStackSize += 8;
    return Datatype(DatatypeCategory::i64);
}
Datatype Compiler::compileLiteralBool(bool value) {
#ifdef x86_64_BIT_MODE
    addInstructions(Instruction::PUSH_8, value, 0);
    mStackSize += 8;
#else
    addInstructionOneByteParam(Instruction::PUSH_1, value);
    mStackSize += 1;
#endif
    return Datatype(DatatypeCategory::bool_);
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

        case BinaryExpressionNode::BinaryOperator::COMPARISON_MORE_THAN:
            addInstructions(Instruction::COMPARE_MORE_THAN_I32);
            mStackSize -= getSimpleSize(DatatypeCategory::i32) * 2;
            mStackSize += getSimpleSize(DatatypeCategory::bool_);
            return Datatype{ DatatypeCategory::bool_ };
        }
        break;
    case DatatypeCategory::i64:
        switch(binaryExpression.getOperator()) {
        case BinaryExpressionNode::BinaryOperator::PLUS:
            addInstructions(Instruction::ADD_I64);
            mStackSize -= getSimpleSize(DatatypeCategory::i64);
            return Datatype{ DatatypeCategory::i64 };

        case BinaryExpressionNode::BinaryOperator::MINUS:
            addInstructions(Instruction::SUB_I64);
            mStackSize -= getSimpleSize(DatatypeCategory::i64);
            return Datatype{ DatatypeCategory::i64 };

        case BinaryExpressionNode::BinaryOperator::COMPARISON_LESS_THAN:
            addInstructions(Instruction::COMPARE_LESS_THAN_I64);
            mStackSize -= getSimpleSize(DatatypeCategory::i64) * 2;
            mStackSize += getSimpleSize(DatatypeCategory::bool_);
            return Datatype{ DatatypeCategory::bool_ };

        case BinaryExpressionNode::BinaryOperator::COMPARISON_MORE_THAN:
            addInstructions(Instruction::COMPARE_MORE_THAN_I64);
            mStackSize -= getSimpleSize(DatatypeCategory::i64) * 2;
            mStackSize += getSimpleSize(DatatypeCategory::bool_);
            return Datatype{ DatatypeCategory::bool_ };
        }
        break;
    }
    assert(false);
}
Datatype Compiler::compileIfExpression(const IfExpressionNode& ifExpression) {
    std::vector<int32_t> jumpToEndLabels;
    std::optional<Datatype> returnType;
    for(auto& child : ifExpression.getChildren()) {
        auto conditionType = child.first->compile(*this);
        if(conditionType.getCategory() != DatatypeCategory::bool_) {
            child.first->throwException("Condition for if-expressions must be of type bool, not " + conditionType.toString());
        }
        auto jumpToNextLabel = addLabel(Instruction::JUMP_IF_FALSE);
        mStackSize -= getSimpleSize(DatatypeCategory::bool_);

        auto bodyReturnType = child.second->compile(*this);
        if(returnType) {
            if(*returnType != bodyReturnType) {
                ifExpression.throwException("Each branch of an if-expression must return the same value; other branches returned "
                    + returnType->toString() + ", but one returned " + bodyReturnType.toString());
            }
        } else {
            returnType = bodyReturnType;
        }
        jumpToEndLabels.push_back(addLabel(Instruction::JUMP));
        mStackSize -= bodyReturnType.getSizeOnStack();

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
    std::string fullFunctionName = maybeDeclaration->first;
    auto declarationType = maybeDeclaration->second.type.completeWithTemplateParameters(mTemplateReplacementMap);
    auto declarationAsFunction = dynamic_cast<FunctionDeclarationNode*>(maybeDeclaration->second.astNode);
    if(!declarationAsFunction) {
        // not a normal function, but maybe a native function?
        auto declarationAsNativeFunction = dynamic_cast<NativeFunctionDeclarationNode*>(maybeDeclaration->second.astNode);
        if(!declarationAsNativeFunction) {
            identifier.throwException("Identifier is not a function");
        }
        bool found = false;
        int32_t i = 0;
        for(auto& nativeFunctions : mNativeFunctions) {
            if(nativeFunctions.fullName == fullFunctionName) {
                found = true;
                addInstructions(Instruction::PUSH_8, 3, i);
                mStackSize += 8;
            }
            ++i;
        }
        assert(found);
        return declarationType;
    }
    auto functionTemplateParams = declarationAsFunction->getTemplateParameterVector();
    if(functionTemplateParams.size() != identifier.getTemplateParameters().size()) {
        identifier.throwException("Invalid number of template parameters passed (expected " + std::to_string(functionTemplateParams.size()) + ")");
    }
    if(maybeDeclaration->second.type.hasUndeterminedTemplateTypes()) {
        // replace passed template parameters with those in the current scope: this translates a call like add<T> to add<i32> (if T is currently i32)
        std::vector<Datatype> passedTemplateParameters;
        for(auto& param : identifier.getTemplateParameters()) {
            passedTemplateParameters.push_back(param.completeWithTemplateParameters(mTemplateReplacementMap));
            if(passedTemplateParameters.back().hasUndeterminedTemplateTypes()) {
                identifier.throwException("Passed parameter " + passedTemplateParameters.back().toString() + " couldn't be deduced");
            }
        }
        // template function
        auto replacementMap = createTemplateParamMap(functionTemplateParams, passedTemplateParameters);
        // check if we already compiled this template instantiation (or plan on compiling it) and, if not, add it for later compilation
        bool alreadyExists = false;
        for(auto& func : mProgram.functions) {
            if(func.name == fullFunctionName && func.templateParameters == replacementMap) {
                alreadyExists = true;
            }
        }
        for(auto& plannedFunc : mTemplateFunctionsToInstantiate) {
            if(plannedFunc.function == declarationAsFunction && plannedFunc.replacementMap == replacementMap) {
                alreadyExists = true;
            }
        }
        if(!alreadyExists) {
            mTemplateFunctionsToInstantiate.emplace_back(TemplateFunctionToInstantiate{ declarationAsFunction, replacementMap });
        }
        mLabelsToInsertFunctionIds.emplace_back(FunctionIdInCodeToInsert{ .label = addLabel(Instruction::PUSH_8), .fullFunctionName = fullFunctionName, .templateParameters = replacementMap });
        mStackSize += 8;
        return maybeDeclaration->second.type.completeWithTemplateParameters(replacementMap);
    }
    // normal function
    mLabelsToInsertFunctionIds.emplace_back(FunctionIdInCodeToInsert{ .label = addLabel(Instruction::PUSH_8), .fullFunctionName = fullFunctionName });
    mStackSize += 8;
    return maybeDeclaration->second.type;
}
Datatype Compiler::compileFunctionCall(const FunctionCallExpressionNode& functionCall) {
    auto functionNameType = functionCall.getName()->compile(*this);
    if(functionNameType.getCategory() != DatatypeCategory::function) {
        functionCall.throwException("Calling non-function type " + functionNameType.toString());
    }
    auto& functionInfo = functionNameType.getFunctionTypeInfo();
    if(functionCall.getParams().size() != functionInfo.second.size()) {
        functionCall.throwException("Calling function with invalid number of arguments; function is of type "
            + functionNameType.toString() + ", but " + std::to_string(functionCall.getParams().size()) + " arguments were passed");
    }
    size_t i = 0;
    int32_t paramTypesSummedSize = 0;
    for(auto& param : functionCall.getParams()) {
        auto type = param->compile(*this);
        paramTypesSummedSize += type.getSizeOnStack();
        if(functionInfo.second.at(i) != type) {
            functionCall.throwException("Calling function with invalid arguments; argument at index "
                + std::to_string(i) + " should be an " + functionInfo.second.at(i).toString() + ", but is a " + type.toString());
        }
        ++i;
    }

    addInstructions(Instruction::CALL, paramTypesSummedSize);
    mStackSize -= paramTypesSummedSize + functionNameType.getSizeOnStack();
    mStackSize += functionInfo.first->getSizeOnStack();

    return Datatype{ *functionInfo.first };
}
Datatype Compiler::compileChainedFunctionCall(const FunctionChainExpressionNode& functionChain) {
    auto initialValueType = functionChain.getInitialValue()->compile(*this);
    auto& functionCall = *functionChain.getFunctionCall();
    auto functionNameType = functionCall.getName()->compile(*this);
    if(functionNameType.getCategory() != DatatypeCategory::function) {
        functionCall.throwException("Calling non-function type " + functionNameType.toString());
    }
    auto& functionInfo = functionNameType.getFunctionTypeInfo();
    if(functionCall.getParams().size() + 1 != functionInfo.second.size()) {
        functionCall.throwException("Calling function with invalid number of arguments; function is of type "
            + functionNameType.toString() + ", but " + std::to_string(functionCall.getParams().size() + 1) + " arguments were passed (one chained)");
    }
    // move initial value back to the top of the stack
    addInstructions(Instruction::REPUSH_FROM_N, initialValueType.getSizeOnStack(), 8);
    addInstructions(Instruction::POP_N_BELOW, initialValueType.getSizeOnStack(), 8 + initialValueType.getSizeOnStack());
    size_t i = 1;
    int32_t paramTypesSummedSize = initialValueType.getSizeOnStack();
    for(auto& param : functionCall.getParams()) {
        auto type = param->compile(*this);
        paramTypesSummedSize += type.getSizeOnStack();
        if(functionInfo.second.at(i) != type) {
            functionCall.throwException("Calling function with invalid arguments; argument at index "
                + std::to_string(i) + " should be an " + functionInfo.second.at(i).toString() + ", but is a " + type.toString());
        }
        ++i;
    }

    addInstructions(Instruction::CALL, paramTypesSummedSize);
    mStackSize -= paramTypesSummedSize + functionNameType.getSizeOnStack();
    mStackSize += functionInfo.first->getSizeOnStack();

    return Datatype{ *functionInfo.first };
}
Datatype Compiler::compileTupleCreationExpression(const TupleCreationNode& tupleCreation) {
    std::vector<Datatype> paramTypes;
    for(auto& param : tupleCreation.getParams()) {
        paramTypes.emplace_back(param->compile(*this));
    }
    return Datatype{ std::move(paramTypes) };
}
Datatype Compiler::compileTupleAccessExpression(const TupleAccessExpressionNode& tupleAccess) {
    auto tupleType = tupleAccess.getName()->compile(*this);
    if(tupleType.getCategory() != DatatypeCategory::tuple) {
        tupleAccess.throwException("Trying to access a tuple-element of non-tuple type " + tupleType.toString());
    }
    auto& tupleInfo = tupleType.getTupleInfo();
    auto& accessedType = tupleInfo.at(tupleAccess.getIndex());

    size_t offsetOfAccessedType = 0;
    for(ssize_t i = static_cast<ssize_t>(tupleInfo.size()) - 1; i > tupleAccess.getIndex(); --i) {
        offsetOfAccessedType += tupleInfo.at(i).getSizeOnStack();
    }
    addInstructions(Instruction::REPUSH_FROM_N, accessedType.getSizeOnStack(), offsetOfAccessedType);
    addInstructions(Instruction::POP_N_BELOW, tupleType.getSizeOnStack(), accessedType.getSizeOnStack());

    mStackSize += accessedType.getSizeOnStack();
    mStackSize -= tupleType.getSizeOnStack();
    return accessedType;
}
Datatype Compiler::compileAssignmentExpression(const AssignmentExpression& assignment) {
    auto rhsType = assignment.getRight()->compile(*this);
    auto& lhs = *assignment.getLeft();
    addInstructions(Instruction::REPUSH_FROM_N, rhsType.getSizeOnStack(), 0);
    mStackSize += rhsType.getSizeOnStack();
    saveVariableLocation(lhs.getName(), rhsType);
    mStackFrames.top().stackFrameSize += rhsType.getSizeOnStack();
    return rhsType;
}
Datatype Compiler::compileLambdaCreationExpression(const LambdaCreationNode& node) {
    std::vector<const IdentifierNode*> usedIdentifiersInLambda;
    VariableSearcher searcher{ usedIdentifiersInLambda };
    node.getBody()->findUsedVariables(searcher);

    // push ip of lambda
    mLabelsToInsertLambdaIds.emplace_back(LambdaIdInCodeToInsert{ .label = addLabel(Instruction::PUSH_8), .lambda = &node });
    mStackSize += 8;

    // push lambda parameters outside of this scope
    std::vector<std::pair<std::string, Datatype>> usedIdentifiersWithType;
    int32_t sizeOfCopiedScopeValues = 0;
    for(auto& identifier : usedIdentifiersInLambda) {
        // check if the identifier is a parameter of the lambda
        bool isParameter = false;
        for(auto& param : node.getParameters()) {
            if(param.name->getName() == identifier->getName()) {
                isParameter = true;
            }
        }
        // if not, check in the surrounding scope
        if(!isParameter) {
            try {
                auto identifierType = compileIdentifierLoad(*identifier);
                sizeOfCopiedScopeValues += identifierType.getSizeOnStack();
                usedIdentifiersWithType.emplace_back(identifier->getName(), identifierType);
            } catch(std::exception& e) {
                node.throwException("While looking for the identifiers used, the following exception occurred:\n" + std::string{ e.what() });
            }
        }
    }

    // create the lambda, moving all previously copied parameters to the heap and storing the pointer in the
    // upper 4 bytes of the ip of the lambda (we only need for, but reserve 8)
    addInstructions(Instruction::CREATE_LAMBDA, sizeOfCopiedScopeValues);
    mStackSize -= sizeOfCopiedScopeValues + 8;
    mStackSize += 8;
    mLambdasToCompile.emplace(LambdaToCompile{ .lambda = &node, .copiedParameters = std::move(usedIdentifiersWithType) });

    std::vector<Datatype> paramTypes;
    for(auto& param : node.getParameters()) {
        paramTypes.push_back(param.type);
    }
    return Datatype(node.getReturnType(), std::move(paramTypes)).completeWithTemplateParameters(mTemplateReplacementMap);
}
Program::Function& Compiler::compileLambda(const LambdaToCompile& lambdaToCompile) {
    std::vector<std::pair<std::string, Datatype>> parameters;
    for(auto& realParam : lambdaToCompile.lambda->getParameters()) {
        parameters.emplace_back(realParam.name->getName(), realParam.type);
    }
    parameters.insert(parameters.end(), lambdaToCompile.copiedParameters.begin(), lambdaToCompile.copiedParameters.end());
    return compileFunctionlikeThing("lambda", lambdaToCompile.lambda->getReturnType(), parameters, *lambdaToCompile.lambda->getBody(), IsLambda::Yes);
}
Program::Function& Compiler::compileFunctionlikeThing(const std::string& fullFunctionName, const Datatype& returnType, const std::vector<std::pair<std::string, Datatype>>& params, const ScopeNode& body, IsLambda isLambda) {
    printf("\nCompiling function %s\n", fullFunctionName.c_str());

    auto start = mProgram.code.size();
    pushStackFrame();

    mCurrentStackInfoTree = std::make_unique<StackInformationTree>(start, mStackSize, StackInformationTree::IsAtPopInstruction::No);
    mCurrentStackInfoTreeNode = mCurrentStackInfoTree.get();

    const auto& functionReturnType = returnType.completeWithTemplateParameters(mTemplateReplacementMap);
    for(const auto& param : params) {
        auto type = param.second.completeWithTemplateParameters(mTemplateReplacementMap);
        mStackSize += type.getSizeOnStack();
        saveVariableLocation(param.first, type);
    }
    mStackFrames.top().stackFrameSize = mStackSize;
    auto bodyReturnType = body.compile(*this);
    if(bodyReturnType != functionReturnType) {
        throw std::runtime_error{ "Function's declared return type " + functionReturnType.toString() + " and actual return type " + bodyReturnType.toString() + " don't match" };
    }

    // pop all parameters of the stack
    {
        auto bytesToPop = mStackFrames.top().stackFrameSize;
        if(bytesToPop > 0) {
            addInstructions(Instruction::POP_N_BELOW, bytesToPop, functionReturnType.getSizeOnStack());
            mStackSize -= bytesToPop;
        }
        mCurrentStackInfoTreeNode->addChild(std::make_unique<StackInformationTree>(mProgram.code.size(), mStackSize, StackInformationTree::IsAtPopInstruction::Yes));
        mStackFrames.pop();
    }
    assert(mStackSize == static_cast<int32_t>(functionReturnType.getSizeOnStack()));
    addInstructions(Instruction::RETURN, functionReturnType.getSizeOnStack());
    mStackSize = 0;

    // figure out type of function
    std::vector<Datatype> parameterTypes;
    parameterTypes.reserve(params.size());
    for(auto& param : params) {
        parameterTypes.emplace_back(param.second);
    }
    Datatype functionType{ returnType, std::move(parameterTypes) };

    // save the region of the function in the program object to allow locating it
    auto& entry = mProgram.functions.emplace_back(Program::Function{
        .offset = static_cast<int32_t>(start),
        .len = static_cast<int32_t>(mProgram.code.size() - start),
        .type = functionType,
        .name = fullFunctionName,
        .templateParameters = mTemplateReplacementMap,
        .stackInformation = std::move(mCurrentStackInfoTree),
        .stackSizePerIp = std::move(mIpToStackSize) });
    assert(mCurrentStackInfoTreeNode);
    assert(mCurrentStackInfoTreeNode->getParent() == nullptr);
    mCurrentStackInfoTreeNode = nullptr;
    mIpToStackSize = {};
    return entry;
}
void Compiler::saveVariableLocation(std::string name, Datatype type) {
    mStackFrames.top().variables.emplace(name, VariableOnStack{ .offsetFromBottom = mStackSize, .type = type });
    mCurrentStackInfoTreeNode->addChild(std::make_unique<StackInformationTree>(mProgram.code.size(), mStackSize, name, std::move(type)));
}
void Compiler::saveCurrentStackSizeToDebugInfo() {
    mIpToStackSize.emplace(mProgram.code.size(), mStackSize);
}

VariableSearcher::VariableSearcher(std::vector<const IdentifierNode*>& identifiers)
: mIdentifiers(identifiers) {
}
void VariableSearcher::identifierFound(const IdentifierNode& identifier) {
    mIdentifiers.push_back(&identifier);
}
}