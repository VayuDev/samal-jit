#pragma once
#include "Datatype.hpp"
#include <string>

namespace samal {

class IdentifierNode;
class ExpressionNode;

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
    up<IdentifierNode> name;
    Datatype type;
  };
  explicit ParameterListNode(std::vector<Parameter> params);
  [[nodiscard]] std::string dump(unsigned indent) const override;
  [[nodiscard]] inline const char* getClassName() const override { return "ParameterListNode"; }
 private:
  std::vector<Parameter> mParams;
};

class ParameterListNodeWithoutDatatypes : public ASTNode {
 public:
  explicit ParameterListNodeWithoutDatatypes(std::vector<up<ExpressionNode>> params);
  [[nodiscard]] std::string dump(unsigned indent) const override;
  [[nodiscard]] inline const char* getClassName() const override { return "ParameterListNodeWithoutDatatypes"; }
 private:
  std::vector<up<ExpressionNode>> mParams;
};


class ExpressionNode : public ASTNode {
 public:
  [[nodiscard]] virtual std::optional<Datatype> getDatatype() const = 0;
  [[nodiscard]] inline const char* getClassName() const override { return "ExpressionNode"; }
 private:
};

class BinaryExpressionNode : public ExpressionNode {
 public:
  enum class BinaryOperator {
    PLUS,
    MINUS,
    MULTIPLY,
    DIVIDE
  };
  BinaryExpressionNode(up<ExpressionNode> left, BinaryOperator op, up<ExpressionNode> right);
  [[nodiscard]] std::optional<Datatype> getDatatype() const override;
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
  [[nodiscard]] std::optional<Datatype> getDatatype() const override;
  [[nodiscard]] std::string dump(unsigned indent) const override;
  [[nodiscard]] inline const char* getClassName() const override { return "LiteralIntNode"; }
 private:
  int32_t mValue;
};

class IdentifierNode : public ExpressionNode {
 public:
  explicit IdentifierNode(std::string name);
  [[nodiscard]] std::optional<Datatype> getDatatype() const override;
  [[nodiscard]] std::string dump(unsigned indent) const override;
  [[nodiscard]] inline const char* getClassName() const override { return "IdentifierNode"; }
 private:
  std::string mName;
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

using IfExpressionChildList = std::vector<std::pair<up<ExpressionNode>, up<ScopeNode>>>;
class IfExpressionNode : public ExpressionNode {
 public:
  IfExpressionNode(IfExpressionChildList children, up<ScopeNode> elseBody);
  [[nodiscard]] std::optional<Datatype> getDatatype() const override;
  [[nodiscard]] std::string dump(unsigned indent) const override;
  [[nodiscard]] inline const char* getClassName() const override { return "IfExpressionNode"; }
 private:
  IfExpressionChildList mChildren;
  up<ScopeNode> mElseBody;
};

class FunctionCallExpressionNode : public ExpressionNode {
 public:
  FunctionCallExpressionNode(up<IdentifierNode> name, up<ParameterListNodeWithoutDatatypes> params);
  [[nodiscard]] std::optional<Datatype> getDatatype() const override;
  [[nodiscard]] std::string dump(unsigned indent) const override;
  [[nodiscard]] inline const char* getClassName() const override { return "FunctionCallExpressionNode"; }
 private:
  up<IdentifierNode> mName;
  up<ParameterListNodeWithoutDatatypes> mParams;
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
  FunctionDeclarationNode(up<IdentifierNode> name, up<ParameterListNode> params, Datatype returnType, up<ScopeNode> body);
  [[nodiscard]] std::string dump(unsigned indent) const override;
  [[nodiscard]] inline const char* getClassName() const override { return "FunctionDeclarationNode"; }
 private:
  up<IdentifierNode> mName;
  up<ParameterListNode> mParameters;
  Datatype mReturnType;
  up<ScopeNode> mBody;
};

}