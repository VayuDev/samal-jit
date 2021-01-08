#pragma once
#include "Instruction.hpp"
#include "Forward.hpp"
#include <vector>
#include <stack>
#include <map>
#include "Util.hpp"
#include "AST.hpp"
#include "Program.hpp"

namespace samal {

class FunctionDuration final {
 public:
  FunctionDuration(Compiler& compiler, const up<IdentifierNode>& identifier, const up<ParameterListNode>& params);
  ~FunctionDuration();
 private:
  Compiler& mCompiler;
  const up<IdentifierNode>& mIdentifier;
  const up<ParameterListNode>& mParams;
};

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
  Program compile(std::vector<up<ModuleRootNode>>& modules);
  void assignToVariable(const up<IdentifierNode>& identifier);
  [[nodiscard]] FunctionDuration enterFunction(const up<IdentifierNode>& identifier, const up<ParameterListNode>& params);
  [[nodiscard]] ScopeDuration enterScope(const Datatype& returnType);

  size_t addLabel(size_t len);
  void* getLabelPtr(size_t label);
  size_t getCurrentLocation();
  void changeStackSize(ssize_t diff);

  template<typename T>
  inline void pushPrimitiveLiteral(T) {
    assert(false);
  }
  template<>
  inline void pushPrimitiveLiteral(int32_t param) {
#ifdef x86_64_BIT_MODE
    addInstructions(Instruction::PUSH_8, param, 0);
    mStackSize += 8;
#else
    addInstructions(Instruction::PUSH_4, param);
    mStackSize += 4;
#endif
  }
  void loadVariableToStack(const IdentifierNode& identifier);
  void popUnusedValueAtEndOfScope(const Datatype& type);
  void binaryOperation(const Datatype& inputTypes, BinaryExpressionNode::BinaryOperator op);
  void performFunctionCall(size_t sizeOfArguments, size_t sizeOfReturnValue);
  void setVariableLocation(const up<IdentifierNode>& identifier);
 private:
  void addInstructions(Instruction insn);
  void addInstructions(Instruction insn, int32_t param);
  void addInstructions(Instruction insn, int32_t param1, int32_t param2);
  struct VariableInfoOnStack {
    size_t offsetFromTop;
    size_t sizeOnStack;
  };

  struct StackFrame {
    std::map<int32_t, VariableInfoOnStack> variables;
    size_t bytesToPopOnExit { 0 };
  };

  std::optional<Program> mProgram;
  std::stack<StackFrame> mStackFrames;
  int mStackSize { 0 };
  std::map<int32_t, int32_t> mFunctions;

  friend class FunctionDuration;
  friend class ScopeDuration;
};

}