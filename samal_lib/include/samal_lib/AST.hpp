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

class ExpressionNode : public ASTNode {
 public:
  [[nodiscard]] virtual std::optional<Datatype> getDatatype() const = 0;
  [[nodiscard]] inline const char* getClassName() const override { return "ExpressionNode"; }
 private:
};

class BinaryExpressionNode : public ASTNode {
 public:
  enum class BinaryOperator {
    PLUS,
    MINUS,
    MULTIPLY,
    DIVIDE
  };
  BinaryExpressionNode(up<ExpressionNode> left, BinaryOperator op, up<ExpressionNode> right);
  [[nodiscard]] virtual std::optional<Datatype> getDatatype() const;
  [[nodiscard]] std::string dump(unsigned indent) const override;
  [[nodiscard]] inline const char* getClassName() const override { return "BinaryExpressionNode"; }
 private:
  up<ExpressionNode> mLeft;
  BinaryOperator mOperator;
  up<ExpressionNode> mRight;
};

class LiteralNode : public ExpressionNode {
 public:

  [[nodiscard]] inline const char* getClassName() const override { return "LiteralNode"; }
 private:
};

class LiteralInt32Node : public LiteralNode {
 public:
  explicit LiteralInt32Node(int32_t val);
  [[nodiscard]] virtual std::optional<Datatype> getDatatype() const;
  [[nodiscard]] std::string dump(unsigned indent) const override;
  [[nodiscard]] inline const char* getClassName() const override { return "LiteralIntNode"; }
 private:
  int32_t mValue;
};

class ScopeNode : public ExpressionNode {
 public:
  explicit ScopeNode(std::vector<up<ExpressionNode>> expressions);
  [[nodiscard]] virtual std::optional<Datatype> getDatatype() const;
  [[nodiscard]] std::string dump(unsigned indent) const override;
  [[nodiscard]] inline const char* getClassName() const override { return "ScopeNode"; }
 private:
  std::vector<up<ExpressionNode>> mExpressions;
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

}