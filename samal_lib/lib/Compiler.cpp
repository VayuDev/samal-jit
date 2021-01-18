#include "samal_lib/Compiler.hpp"
#include "samal_lib/AST.hpp"
#include <cstdio>

namespace samal {

Compiler::Compiler() {
}

Program Compiler::compile(std::vector<up<ModuleRootNode>>& modules) {
    mProgram.emplace();
    for(auto& module : modules) {
        module->compile(*this);
    }
    for(auto& locationInCode : mFunctionIdsInCode) {
        auto varId = *(int32_t*)&mProgram->code.at(locationInCode);
        auto ipOffsetOrError = mFunctions.find(varId);
        assert(ipOffsetOrError != mFunctions.end());
        *(int32_t*)&mProgram->code.at(locationInCode) = ipOffsetOrError->second;
    }
    return std::move(*mProgram);
}
void Compiler::assignToVariable(const up<IdentifierNode>& identifier) {
    auto sizeOnStack = identifier->getDatatype()->getSizeOnStack();
    addInstructions(Instruction::REPUSH_N, static_cast<int32_t>(sizeOnStack));
    mStackSize += sizeOnStack;
    mStackFrames.top().variables.emplace(identifier->getId()->variableId, VariableInfoOnStack{ .offsetFromTop = (size_t)mStackSize, .sizeOnStack = sizeOnStack });
}
void Compiler::setVariableLocation(const up<IdentifierNode>& identifier) {
    auto sizeOnStack = identifier->getDatatype()->getSizeOnStack();
    mStackFrames.top().variables.emplace(identifier->getId()->variableId, VariableInfoOnStack{ .offsetFromTop = (size_t)mStackSize, .sizeOnStack = sizeOnStack });
}
void Compiler::addInstructions(Instruction insn, int32_t param) {
    mProgram->code.resize(mProgram->code.size() + 5);
    memcpy(&mProgram->code.at(mProgram->code.size() - 5), &insn, 1);
    memcpy(&mProgram->code.at(mProgram->code.size() - 4), &param, 4);
}
void Compiler::addInstructions(Instruction ins) {
    mProgram->code.resize(mProgram->code.size() + 1);
    memcpy(&mProgram->code.at(mProgram->code.size() - 1), &ins, 1);
}
void Compiler::addInstructions(Instruction insn, int32_t param1, int32_t param2) {
    mProgram->code.resize(mProgram->code.size() + 9);
    memcpy(&mProgram->code.at(mProgram->code.size() - 9), &insn, 1);
    memcpy(&mProgram->code.at(mProgram->code.size() - 8), &param1, 4);
    memcpy(&mProgram->code.at(mProgram->code.size() - 4), &param2, 4);
}
void Compiler::addInstructionOneByteParam(Instruction insn, int8_t param) {
    mProgram->code.resize(mProgram->code.size() + 2);
    memcpy(&mProgram->code.at(mProgram->code.size() - 2), &insn, 1);
    memcpy(&mProgram->code.at(mProgram->code.size() - 1), &param, 1);
}
FunctionDuration Compiler::enterFunction(const up<IdentifierNode>& identifier, const up<ParameterListNode>& params) {
    return FunctionDuration(*this, identifier, params);
}
ScopeDuration Compiler::enterScope(const Datatype& returnType) {
    return ScopeDuration(*this, returnType);
}
void Compiler::popUnusedValueAtEndOfScope(const Datatype& type) {
    mStackFrames.top().bytesToPopOnExit += type.getSizeOnStack();
}
void Compiler::binaryOperation(const Datatype& inputTypes, BinaryExpressionNode::BinaryOperator op) {
    switch(op) {
    case BinaryExpressionNode::BinaryOperator::PLUS:
        switch(inputTypes.getCategory()) {
        case DatatypeCategory::i32:
            addInstructions(Instruction::ADD_I32);
            mStackSize -= getSimpleSize(DatatypeCategory::i32);
            break;
        case DatatypeCategory::i64:
            addInstructions(Instruction::ADD_I64);
            mStackSize -= getSimpleSize(DatatypeCategory::i64);
            break;
        default:
            assert(false);
        }
        break;
    case BinaryExpressionNode::BinaryOperator::MINUS:
        switch(inputTypes.getCategory()) {
        case DatatypeCategory::i32:
            addInstructions(Instruction::SUB_I32);
            mStackSize -= getSimpleSize(DatatypeCategory::i32);
            break;
        case DatatypeCategory::i64:
            addInstructions(Instruction::SUB_I64);
            mStackSize -= getSimpleSize(DatatypeCategory::i64);
            break;
        default:
            assert(false);
        }
        break;
    case BinaryExpressionNode::BinaryOperator::COMPARISON_LESS_THAN:
        switch(inputTypes.getCategory()) {
        case DatatypeCategory::i32:
            addInstructions(Instruction::COMPARE_LESS_THAN_I32);
            mStackSize -= getSimpleSize(DatatypeCategory::i32) * 2 - getSimpleSize(DatatypeCategory::bool_);
            break;
        case DatatypeCategory::i64:
            addInstructions(Instruction::COMPARE_LESS_THAN_I64);
            mStackSize -= getSimpleSize(DatatypeCategory::i64) * 2 - getSimpleSize(DatatypeCategory::bool_);
            break;
        default:
            assert(false);
        }
        break;
    case BinaryExpressionNode::BinaryOperator::COMPARISON_LESS_EQUAL_THAN:
        switch(inputTypes.getCategory()) {
        case DatatypeCategory::i32:
            addInstructions(Instruction::COMPARE_LESS_EQUAL_THAN_I32);
            mStackSize -= getSimpleSize(DatatypeCategory::i32) * 2 - getSimpleSize(DatatypeCategory::bool_);
            break;
        case DatatypeCategory::i64:
            addInstructions(Instruction::COMPARE_LESS_EQUAL_THAN_I64);
            mStackSize -= getSimpleSize(DatatypeCategory::i64) * 2 - getSimpleSize(DatatypeCategory::bool_);
            break;
        default:
            assert(false);
        }
        break;
    case BinaryExpressionNode::BinaryOperator::COMPARISON_MORE_THAN:
        switch(inputTypes.getCategory()) {
        case DatatypeCategory::i32:
            addInstructions(Instruction::COMPARE_MORE_THAN_I32);
            mStackSize -= getSimpleSize(DatatypeCategory::i32) * 2 - getSimpleSize(DatatypeCategory::bool_);
            break;
        case DatatypeCategory::i64:
            addInstructions(Instruction::COMPARE_MORE_THAN_I64);
            mStackSize -= getSimpleSize(DatatypeCategory::i64) * 2 - getSimpleSize(DatatypeCategory::bool_);
            break;
        default:
            assert(false);
        }
        break;
    case BinaryExpressionNode::BinaryOperator::COMPARISON_MORE_EQUAL_THAN:
        switch(inputTypes.getCategory()) {
        case DatatypeCategory::i32:
            addInstructions(Instruction::COMPARE_MORE_EQUAL_THAN_I32);
            mStackSize -= getSimpleSize(DatatypeCategory::i32) * 2 - getSimpleSize(DatatypeCategory::bool_);
            break;
        case DatatypeCategory::i64:
            addInstructions(Instruction::COMPARE_MORE_EQUAL_THAN_I64);
            mStackSize -= getSimpleSize(DatatypeCategory::i64) * 2 - getSimpleSize(DatatypeCategory::bool_);
            break;
        default:
            assert(false);
        }
        break;
    default:
        assert(false);
    }
}
void Compiler::loadVariableToStack(const IdentifierNode& identifier) {
    auto stackCpy = mStackFrames;
    while(!stackCpy.empty()) {
        auto varLocation = stackCpy.top().variables.find(identifier.getId()->variableId);
        if(varLocation != stackCpy.top().variables.end()) {
            assert(identifier.getId()->templateId == 0);
            addInstructions(Instruction::REPUSH_FROM_N, varLocation->second.sizeOnStack, mStackSize - varLocation->second.offsetFromTop);
            // replace every function id with its ip at the end
            if(identifier.getDatatype()->getCategory() == DatatypeCategory::function) {
                mFunctionIdsInCode.push_back(mProgram->code.size() - 8);
            }
            mStackSize += varLocation->second.sizeOnStack;
            return;
        }
        stackCpy.pop();
    }
    if(identifier.getDatatype()->getCategory() == DatatypeCategory::function) {
        addInstructions(Instruction::PUSH_8, identifier.getId()->variableId, identifier.getId()->templateId);
        mFunctionIdsInCode.push_back(mProgram->code.size() - 8);
        mStackSize += 8;
        return;
    }
    assert(false);
}
void Compiler::performFunctionCall(size_t sizeOfArguments, size_t sizeOfReturnValue) {
    addInstructions(Instruction::CALL, sizeOfArguments);
    mStackSize -= sizeOfArguments + 8;
    mStackSize += sizeOfReturnValue;
}
size_t Compiler::addLabel(size_t len) {
    mProgram->code.resize(mProgram->code.size() + len);
    return mProgram->code.size() - len;
}
size_t Compiler::getCurrentLocation() {
    return mProgram->code.size();
}
void* Compiler::getLabelPtr(size_t label) {
    return &mProgram->code.at(label);
}
void Compiler::changeStackSize(ssize_t diff) {
    mStackSize += diff;
}
void Compiler::accessTupleElement(const Datatype& tupleType, uint32_t index) {
    auto& tupleInfo = tupleType.getTupleInfo();
    auto& accessedType = tupleInfo.at(index);
    size_t offsetOfAccessedType = 0;
    for(ssize_t i = static_cast<ssize_t>(tupleInfo.size()) - 1; i > index; --i) {
        offsetOfAccessedType += tupleInfo.at(i).getSizeOnStack();
    }
    addInstructions(Instruction::REPUSH_FROM_N, accessedType.getSizeOnStack(), offsetOfAccessedType);
    addInstructions(Instruction::POP_N_BELOW, tupleType.getSizeOnStack(), accessedType.getSizeOnStack());

    mStackSize += accessedType.getSizeOnStack();
    mStackSize -= tupleType.getSizeOnStack();
}
void Compiler::moveToTop(size_t len, size_t offset) {
    addInstructions(Instruction::REPUSH_FROM_N, len, offset);
    addInstructions(Instruction::POP_N_BELOW, len, offset + len);
}

FunctionDuration::FunctionDuration(Compiler& compiler, const up<IdentifierNode>& identifier, const up<ParameterListNode>& params)
: mCompiler(compiler), mIdentifier(identifier), mParams(params) {
    auto functionId = mIdentifier->getId();
    assert(functionId);
    mCompiler.mFunctions.emplace(functionId->variableId, mCompiler.getCurrentLocation());
    mCompiler.mProgram->functions.emplace(std::make_pair(mIdentifier->getName(), Program::Function{ .offset = static_cast<int32_t>(mCompiler.mProgram->code.size()), .len = -1, .type = *mIdentifier->getDatatype() }));
    mCompiler.mStackFrames.emplace();

    for(auto& param : mParams->getParams()) {
        mCompiler.mStackSize += param.type.getSizeOnStack();
        mCompiler.setVariableLocation(param.name);
    }
}
FunctionDuration::~FunctionDuration() {
    // the stackframe contains the allocation for the parameters
    assert(mCompiler.mStackFrames.top().bytesToPopOnExit == 0);
    size_t sumSize = 0;
    for(auto& var : mCompiler.mStackFrames.top().variables) {
        sumSize += var.second.sizeOnStack;
    }
    const auto functionType = mIdentifier->getDatatype();
    assert(functionType);
    const auto returnTypeSize = functionType->getFunctionTypeInfo().first->getSizeOnStack();
    if(sumSize > 0) {
        mCompiler.addInstructions(Instruction::POP_N_BELOW, static_cast<int32_t>(sumSize), returnTypeSize);
        mCompiler.mStackSize -= sumSize;
    }
    mCompiler.addInstructions(Instruction::RETURN, returnTypeSize);
    mCompiler.mStackFrames.pop();
    mCompiler.mProgram->functions.at(mIdentifier->getName()).len = mCompiler.mProgram->code.size() - mCompiler.mProgram->functions.at(mIdentifier->getName()).offset;

    // TODO won't work for lambdas I guess
    assert(mCompiler.mStackSize == returnTypeSize);
    mCompiler.mStackSize = 0;
}
ScopeDuration::ScopeDuration(Compiler& compiler, const Datatype& returnType)
: mCompiler(compiler) {
    mCompiler.mStackFrames.emplace();
    mReturnTypeSize = returnType.getSizeOnStack();
}
ScopeDuration::~ScopeDuration() {
    // At the end of the scope, we need to pop all local variables off the stack.
    // We do this in a single instruction at the end to improve performance.
    size_t sumSize = mCompiler.mStackFrames.top().bytesToPopOnExit;
    for(auto& var : mCompiler.mStackFrames.top().variables) {
        sumSize += var.second.sizeOnStack;
    }
    if(sumSize > 0) {
        mCompiler.addInstructions(Instruction::POP_N_BELOW, static_cast<int32_t>(sumSize), mReturnTypeSize);
        mCompiler.mStackSize -= sumSize;
    }
    mCompiler.mStackFrames.pop();
}

std::string Program::disassemble() const {
    std::string ret;
    for(auto& [name, location] : functions) {
        ret += "Function " + name + ":\n";
        size_t offset = location.offset;
        while(offset < location.offset + location.len) {
            ret += " " + std::to_string(offset) + " ";
            ret += instructionToString(static_cast<Instruction>(code.at(offset)));
            auto width = instructionToWidth(static_cast<Instruction>(code.at(offset)));
            if(width >= 5) {
                ret += " ";
                ret += std::to_string(*reinterpret_cast<const int32_t*>(&code.at(offset + 1)));
            }
            if(width >= 9) {
                ret += " ";
                ret += std::to_string(*reinterpret_cast<const int32_t*>(&code.at(offset + 5)));
            }
            ret += "\n";
            offset += width;
        }
        ret += "\n";
    }
    return ret;
}
}