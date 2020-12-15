#pragma once
#include "Datatype.hpp"
#include <string>

namespace samal {

class ASTNode {
 public:
  virtual ~ASTNode() = default;
 private:
};

class DeclarationNode : public ASTNode {
 public:

 private:
};

class ModuleRootNode : public ASTNode {
 public:
  explicit ModuleRootNode(std::vector<up<DeclarationNode>>&& declarations);
 private:
  std::vector<up<DeclarationNode>> mDeclarations;
};

class FunctionDeclarationNode : public DeclarationNode {
 public:
  struct Parameter {
    Datatype type;
    std::string name;
  };
 private:
  Datatype mReturnType;
  std::vector<Parameter> mParameters;
};

class ExpressionNode : public ASTNode {
 public:
  [[nodiscard]] virtual Datatype getDatatype() const = 0;
 private:
};

class LiteralNode : public ExpressionNode {
 public:

 private:
};

class LiteralIntNode : public LiteralNode {
 public:

 private:
};

}