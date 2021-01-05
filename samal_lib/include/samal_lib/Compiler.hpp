#pragma once
#include "Instruction.hpp"
#include "Forward.hpp"
#include <vector>
#include <stack>
#include <map>
#include "Util.hpp"
#include "AST.hpp"

namespace samal {

struct Program {
  std::vector<std::vector<uint8_t>> code;
  [[nodiscard]] std::string disassemble() const;
};

class FunctionDuration final {
 public:
  FunctionDuration(Compiler& compiler, const up<IdentifierNode>& identifier);
  ~FunctionDuration();
 private:
  Compiler& mCompiler;
  const up<IdentifierNode>& mIdentifier;
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
  [[nodiscard]] FunctionDuration enterFunction(const up<IdentifierNode>& identifier);
  [[nodiscard]] ScopeDuration enterScope(const Datatype& returnType);
  void setVariableLocation(const up<IdentifierNode>& identifier, size_t offsetFromTop);

  size_t addLabel(size_t len);
  void* getLabelPtr(size_t label);
  size_t getCurrentLocation();

  template<typename T>
  inline void pushPrimitiveLiteral(T param) {
    assert(false);
  }
  template<>
  inline void pushPrimitiveLiteral(int32_t param) {
    addInstructions(Instruction::PUSH_4, param);
    mStackSize += 4;
  }
  void loadVariableToStack(const IdentifierNode& identifier);
  void popUnusedValueAtEndOfScope(const Datatype& type);
  void binaryOperation(const Datatype& inputTypes, BinaryExpressionNode::BinaryOperator op);
  void performFunctionCall(size_t sizeOfArguments, size_t sizeOfReturnValue);
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
  size_t mStackSize { 0 };
  std::vector<uint8_t> *currentFunction { nullptr };

  friend class FunctionDuration;
  friend class ScopeDuration;
};

}