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
  mStackFrames.top().variables.emplace(*identifier->getId(), VariableInfoOnStack{.offsetFromTop = mStackSize - sizeOnStack, .sizeOnStack = sizeOnStack});
}
void Compiler::addInstructions(Instruction insn, int32_t param) {
  assert(currentFunction);
  currentFunction->resize(currentFunction->size() + 5);
  memcpy(&currentFunction->at(currentFunction->size() - 5), &insn, 1);
  memcpy(&currentFunction->at(currentFunction->size() - 4), &param, 4);
}
FunctionDuration Compiler::enterFunction(const up<IdentifierNode> &identifier) {
  return FunctionDuration(*this, identifier);
}
ScopeDuration Compiler::enterScope() {
  return ScopeDuration(*this);
}

void Compiler::setVariableLocation(const up<IdentifierNode> &identifier, size_t offsetFromTop) {
  mStackFrames.top().variables.emplace(*identifier->getId(), VariableInfoOnStack{.offsetFromTop = offsetFromTop, .sizeOnStack = identifier->getDatatype()->getSizeOnStack()});
}
void Compiler::popUnusedValueAtEndOfScope(const Datatype& type) {
  mStackFrames.top().bytesToPopOnExit += type.getSizeOnStack();
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
ScopeDuration::ScopeDuration(Compiler &compiler)
: mCompiler(compiler) {
  mCompiler.mStackFrames.emplace();
}
ScopeDuration::~ScopeDuration() {
  // At the end of the scope, we need to pop all local variables off the stack.
  // We do this in a single instruction at the end to improve performance.
  size_t sumSize = mCompiler.mStackFrames.top().bytesToPopOnExit;
  for(auto& var: mCompiler.mStackFrames.top().variables) {
    sumSize += var.second.sizeOnStack;
  }
  if(sumSize > 0) {
    mCompiler.addInstructions(Instruction::POP_N, static_cast<int32_t>(sumSize));
    mCompiler.mStackSize -= sumSize;
  }
  mCompiler.mStackFrames.pop();
}

std::string Program::disassemble() const {
  std::string ret;
  for(size_t i = 0; i < code.size(); ++i) {
    ret += "Function " + std::to_string(i) + ":\n";
    size_t offset = 0;
    while(offset < code.at(i).size()) {
      ret += instructionToString(static_cast<Instruction>(code.at(i).at(offset)));
      auto width = instructionToWidth(static_cast<Instruction>(code.at(i).at(offset)));
      if(width == 5) {
        ret += " ";
        ret += std::to_string(*reinterpret_cast<const int32_t*>(&code.at(i).at(offset + 1)));
      }
      ret += "\n";
      offset += width;
    }
    ret += "\n";
  }
  return ret;
}
}