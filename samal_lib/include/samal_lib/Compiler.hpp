#pragma once
#include "Instruction.hpp"
#include "Forward.hpp"
#include <vector>
#include <stack>
#include <map>
#include "Util.hpp"

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
  explicit ScopeDuration(Compiler& compiler);
  ~ScopeDuration();
 private:
  Compiler& mCompiler;
};

class Compiler {
 public:
  Compiler();
  Program compile(std::vector<up<ModuleRootNode>>& modules);
  void assignToVariable(const up<IdentifierNode>& identifier);
  [[nodiscard]] FunctionDuration enterFunction(const up<IdentifierNode>& identifier);
  [[nodiscard]] ScopeDuration enterScope();
  void setVariableLocation(const up<IdentifierNode>& identifier, size_t offsetFromTop);

  template<typename T>
  inline void pushPrimitiveLiteral(T param) {
    assert(false);
  }
  template<>
  inline void pushPrimitiveLiteral(int32_t param) {
    addInstructions(Instruction::PUSH_4, param);
    mStackSize += 4;
  }
  void popUnusedValueAtEndOfScope(const Datatype& type);
 private:
  void addInstructions(Instruction insn, int32_t param);

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