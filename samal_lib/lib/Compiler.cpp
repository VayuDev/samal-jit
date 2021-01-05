#include <cstdio>
#include "samal_lib/Compiler.hpp"
#include "samal_lib/AST.hpp"

namespace samal {

Compiler::Compiler() {
}
Program Compiler::compile(std::vector<up<ModuleRootNode>>& modules) {
  mProgram.emplace();
  for(auto& module: modules) {
    module->compile(*this);
  }
  return std::move(*mProgram);
}
void Compiler::assignToVariable(const up<IdentifierNode> &identifier) {
  auto sizeOnStack = identifier->getDatatype()->getSizeOnStack();
  addInstructions(Instruction::REPUSH_N, static_cast<int32_t>(sizeOnStack));
  mStackSize += sizeOnStack;
  mStackFrames.top().variables.emplace(*identifier->getId(), VariableInfoOnStack{.offsetFromTop = mStackSize - sizeOnStack, .sizeOnStack = sizeOnStack});
}
void Compiler::addInstructions(Instruction insn, int32_t param) {
  assert(currentFunction);
  currentFunction->resize(currentFunction->size() + 5);
  memcpy(&currentFunction->at(currentFunction->size() - 5), &insn, 1);
  memcpy(&currentFunction->at(currentFunction->size() - 4), &param, 4);
}
void Compiler::addInstructions(Instruction ins) {
  assert(currentFunction);
  currentFunction->resize(currentFunction->size() + 1);
  memcpy(&currentFunction->at(currentFunction->size() - 1), &ins, 1);
}
void Compiler::addInstructions(Instruction insn, int32_t param1, int32_t param2) {
  assert(currentFunction);
  currentFunction->resize(currentFunction->size() + 9);
  memcpy(&currentFunction->at(currentFunction->size() - 9), &insn, 1);
  memcpy(&currentFunction->at(currentFunction->size() - 8), &param1, 4);
  memcpy(&currentFunction->at(currentFunction->size() - 4), &param2, 4);
}
FunctionDuration Compiler::enterFunction(const up<IdentifierNode> &identifier) {
  return FunctionDuration(*this, identifier);
}
ScopeDuration Compiler::enterScope(const Datatype& returnType) {
  return ScopeDuration(*this, returnType);
}

void Compiler::setVariableLocation(const up<IdentifierNode> &identifier, size_t offsetFromTop) {
  mStackFrames.top().variables.emplace(*identifier->getId(), VariableInfoOnStack{.offsetFromTop = offsetFromTop, .sizeOnStack = identifier->getDatatype()->getSizeOnStack()});
}
void Compiler::popUnusedValueAtEndOfScope(const Datatype& type) {
  mStackFrames.top().bytesToPopOnExit += type.getSizeOnStack();
}
void Compiler::binaryOperation(const Datatype &inputTypes, BinaryExpressionNode::BinaryOperator op) {
  switch(op) {
    case BinaryExpressionNode::BinaryOperator::PLUS:
      switch(inputTypes.getCategory()) {
        case DatatypeCategory::i32:
          addInstructions(Instruction::ADD_I32);
          mStackSize -= 4;
          break;
        default:
          assert(false);
      }
      break;
    case BinaryExpressionNode::BinaryOperator::MINUS:
      switch(inputTypes.getCategory()) {
        case DatatypeCategory::i32:
          addInstructions(Instruction::SUB_I32);
          mStackSize -= 4;
          break;
        default:
          assert(false);
      }
      break;
    case BinaryExpressionNode::BinaryOperator::COMPARISON_LESS_THAN:
      switch(inputTypes.getCategory()) {
        case DatatypeCategory::i32:
          addInstructions(Instruction::COMPARE_LESS_THAN_I32);
          mStackSize -= 8 - 1;
          break;
        default:
          assert(false);
      }
      break;
    case BinaryExpressionNode::BinaryOperator::COMPARISON_LESS_EQUAL_THAN:
      switch(inputTypes.getCategory()) {
        case DatatypeCategory::i32:
          addInstructions(Instruction::COMPARE_LESS_EQUAL_THAN_I32);
          mStackSize -= 8 - 1;
          break;
        default:
          assert(false);
      }
      break;
    case BinaryExpressionNode::BinaryOperator::COMPARISON_MORE_THAN:
      switch(inputTypes.getCategory()) {
        case DatatypeCategory::i32:
          addInstructions(Instruction::COMPARE_MORE_THAN_I32);
          mStackSize -= 8 - 1;
          break;
        default:
          assert(false);
      }
      break;
    case BinaryExpressionNode::BinaryOperator::COMPARISON_MORE_EQUAL_THAN:
      switch(inputTypes.getCategory()) {
        case DatatypeCategory::i32:
          addInstructions(Instruction::COMPARE_MORE_EQUAL_THAN_I32);
          mStackSize -= 8 - 1;
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
    auto varLocation = stackCpy.top().variables.find(*identifier.getId());
    if(varLocation != stackCpy.top().variables.end()) {
      addInstructions(Instruction::REPUSH_FROM_N, identifier.getDatatype()->getSizeOnStack(), mStackSize - varLocation->second.offsetFromTop);
      mStackSize += varLocation->second.sizeOnStack;
      return;
    }
    stackCpy.pop();
  }
  if(identifier.getDatatype()->getCategory() == DatatypeCategory::function) {
    addInstructions(Instruction::PUSH_4, *identifier.getId());
    mStackSize += 4;
    return;
  }
  assert(false);
}
void Compiler::performFunctionCall(size_t sizeOfArguments, size_t sizeOfReturnValue) {
  addInstructions(Instruction::CALL, sizeOfArguments + 4);
  mStackSize -= sizeOfArguments + 4;
  mStackSize += sizeOfReturnValue;
}
size_t Compiler::addLabel(size_t len) {
  assert(currentFunction);
  currentFunction->resize(currentFunction->size() + len);
  return currentFunction->size() - len;
}
size_t Compiler::getCurrentLocation() {
  return currentFunction->size();
}
void *Compiler::getLabelPtr(size_t label) {
  return &currentFunction->at(label);
}

FunctionDuration::FunctionDuration(Compiler &compiler, const up<IdentifierNode> &identifier)
: mCompiler(compiler), mIdentifier(identifier) {
  auto functionId = mIdentifier->getId();
  assert(functionId);
  if(*functionId >= mCompiler.mProgram->code.size()) {
    mCompiler.mProgram->code.resize(*functionId + 1);
  }
  mCompiler.currentFunction = &mCompiler.mProgram->code.at(*functionId);
  mCompiler.mStackFrames.emplace();
}
FunctionDuration::~FunctionDuration() {
  // the stackframe won't contain any allocations, these are in the body
  assert(mCompiler.mStackFrames.top().bytesToPopOnExit == 0);
  auto functionType = mIdentifier->getDatatype();
  mCompiler.addInstructions(Instruction::RETURN, functionType->getFunctionTypeInfo().first->getSizeOnStack());
  mCompiler.mStackFrames.pop();
  mCompiler.currentFunction = nullptr;
}
ScopeDuration::ScopeDuration(Compiler &compiler, const Datatype& returnType)
: mCompiler(compiler) {
  mCompiler.mStackFrames.emplace();
  mReturnTypeSize = returnType.getSizeOnStack();
}
ScopeDuration::~ScopeDuration() {
  // At the end of the scope, we need to pop all local variables off the stack.
  // We do this in a single instruction at the end to improve performance.
  size_t sumSize = mCompiler.mStackFrames.top().bytesToPopOnExit;
  for(auto& var: mCompiler.mStackFrames.top().variables) {
    sumSize += var.second.sizeOnStack;
  }
  if(sumSize > 0) {
    mCompiler.addInstructions(Instruction::POP_N_BELOW, static_cast<int32_t>(sumSize), mReturnTypeSize);
    mCompiler.mStackSize -= sumSize;
  }
  mCompiler.mStackSize -= sumSize;
  mCompiler.mStackFrames.pop();
}

std::string Program::disassemble() const {
  std::string ret;
  for(size_t i = 0; i < code.size(); ++i) {
    ret += "Function " + std::to_string(i) + ":\n";
    size_t offset = 0;
    while(offset < code.at(i).size()) {
      ret += " " + std::to_string(offset) + " ";
      ret += instructionToString(static_cast<Instruction>(code.at(i).at(offset)));
      auto width = instructionToWidth(static_cast<Instruction>(code.at(i).at(offset)));
      if(width >= 5) {
        ret += " ";
        ret += std::to_string(*reinterpret_cast<const int32_t*>(&code.at(i).at(offset + 1)));
      }
      if (width >= 9) {
        ret += " ";
        ret += std::to_string(*reinterpret_cast<const int32_t*>(&code.at(i).at(offset + 5)));
      }
      ret += "\n";
      offset += width;
    }
    ret += "\n";
  }
  return ret;
}
}