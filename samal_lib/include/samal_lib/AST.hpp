#pragma once
#include "Datatype.hpp"
#include <string>

namespace samal {

class ASTNode {
 public:
  virtual ~ASTNode() = default;
  [[nodiscard]] virtual inline const char* getClassName() const { return "ASTNode"; }
  [[nodiscard]] virtual std::string dump(unsigned indent) const;
 private:
};

class ParameterListNode : public ASTNode {
 public:
  struct Parameter {
    std::string name;
    Datatype type;
  };
  explicit ParameterListNode(std::vector<Parameter> params);
  [[nodiscard]] std::string dump(unsigned indent) const override;
  [[nodiscard]] inline const char* getClassName() const override { return "ParameterListNode"; }
 private:
  std::vector<Parameter> mParams;
};

class ScopeNode : public ASTNode {

};


class DeclarationNode : public ASTNode {
 public:

  [[nodiscard]] inline const char* getClassName() const override { return "DeclarationNode"; }
 private:
};

class ModuleRootNode : public ASTNode {
 public:
  explicit ModuleRootNode(std::vector<up<DeclarationNode>>&& declarations);
  [[nodiscard]] std::string dump(unsigned indent) const override;
  [[nodiscard]] inline const char* getClassName() const override { return "ModuleRootNode"; }
 private:
  std::vector<up<DeclarationNode>> mDeclarations;
};

class FunctionDeclarationNode : public DeclarationNode {
 public:
  FunctionDeclarationNode(std::string name, up<ParameterListNode> params, Datatype returnType, up<ScopeNode> body);
  [[nodiscard]] std::string dump(unsigned indent) const override;
  [[nodiscard]] inline const char* getClassName() const override { return "FunctionDeclarationNode"; }
 private:
  std::string mName;
  up<ParameterListNode> mParameters;
  Datatype mReturnType;
  up<ScopeNode> mBody;
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