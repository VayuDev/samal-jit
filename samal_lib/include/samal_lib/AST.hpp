#pragma once
#include "Datatype.hpp"
#include <string>

namespace samal {

class ASTNode {
 public:
  virtual ~ASTNode() = default;
  [[nodiscard]] virtual inline const char* getClassName() const { return "ASTNode"; }
 private:
};

class DeclarationNode : public ASTNode {
 public:

  [[nodiscard]] inline const char* getClassName() const override { return "DeclarationNode"; }
 private:
};

class ModuleRootNode : public ASTNode {
 public:
  explicit ModuleRootNode(std::vector<up<DeclarationNode>>&& declarations);
  [[nodiscard]] inline const char* getClassName() const override { return "ModuleRootNode"; }
 private:
  std::vector<up<DeclarationNode>> mDeclarations;
};

class FunctionDeclarationNode : public DeclarationNode {
 public:
  struct Parameter {
    Datatype type;
    std::string name;
  };
  [[nodiscard]] inline const char* getClassName() const override { return "FunctionDeclarationNode"; }
 private:
  Datatype mReturnType;
  std::vector<Parameter> mParameters;
};

class ExpressionNode : public ASTNode {
 public:
  [[nodiscard]] virtual Datatype getDatatype() const = 0;
  [[nodiscard]] inline const char* getClassName() const override { return "ExpressionNode"; }
 private:
};

class LiteralNode : public ExpressionNode {
 public:

  [[nodiscard]] inline const char* getClassName() const override { return "LiteralNode"; }
 private:
};

class LiteralIntNode : public LiteralNode {
 public:

  [[nodiscard]] inline const char* getClassName() const override { return "LiteralIntNode"; }
 private:
};

}