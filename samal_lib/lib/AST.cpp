#include <cassert>
#include "samal_lib/AST.hpp"
#include "easy_iterator.h"

using namespace easy_iterator;

namespace samal {

static std::string createIndent(unsigned indent) {
  std::string ret;
  for(auto i: range(indent)) {
    ret += ' ';
  }
  return ret;
}

std::string ASTNode::dump(unsigned int indent) const {
  return createIndent(indent) + getClassName() + "\n";
}
ParameterListNode::ParameterListNode(std::vector<Parameter> params)
    : mParams(std::move(params)) {

}
std::string ParameterListNode::dump(unsigned int indent) const {
  auto ret = ASTNode::dump(indent);
  for(auto& child: mParams) {
    ret += createIndent(indent + 1) + child.name + " : " + child.type.toString() + "\n";
  }
  return ret;
}

BinaryExpressionNode::BinaryExpressionNode(up<ExpressionNode> left,
                                            BinaryExpressionNode::BinaryOperator op,
                                            up<ExpressionNode> right)
: mLeft(std::move(left)), mOperator(op), mRight(std::move(right)) {

}
std::optional<Datatype> BinaryExpressionNode::getDatatype() const {
  return Datatype(DatatypeCategory::i32);
}
std::string BinaryExpressionNode::dump(unsigned int indent) const {
  auto ret = ASTNode::dump(indent);
  ret += createIndent(indent + 1) + "Operator: ";
  switch(mOperator) {
    case BinaryOperator::PLUS:
      ret += "+";
      break;
    case BinaryOperator::MINUS:
      ret += "-";
      break;
    case BinaryOperator::MULTIPLY:
      ret += "*";
      break;
    case BinaryOperator::DIVIDE:
      ret += "/";
      break;
    default:
      assert(false);
  }
  ret += "\n";
  ret += createIndent(indent + 1) + "Left:\n" + mLeft->dump(indent + 2);
  ret += createIndent(indent + 1) + "Right:\n" + mRight->dump(indent + 2);
  return ret;
}
LiteralInt32Node::LiteralInt32Node(int32_t val)
: mValue(val) {

}
std::optional<Datatype> LiteralInt32Node::getDatatype() const {
  return Datatype(DatatypeCategory::i32);
}
std::string LiteralInt32Node::dump(unsigned int indent) const {
  return createIndent(indent) + getClassName() + ": " + std::to_string(mValue) + "\n";
}
ScopeNode::ScopeNode(std::vector<up<ExpressionNode>> expressions)
: mExpressions(std::move(expressions)) {

}
std::optional<Datatype> ScopeNode::getDatatype() const {
  return std::optional<Datatype>();
}
std::string ScopeNode::dump(unsigned int indent) const {
  auto ret = ASTNode::dump(indent);
  for(auto& child: mExpressions) {
    ret += child->dump(indent + 1);
  }
  return ret;
}

ModuleRootNode::ModuleRootNode(std::vector<up<DeclarationNode>> &&declarations)
: mDeclarations(std::move(declarations)) {
}
std::string ModuleRootNode::dump(unsigned indent) const {
  auto ret = ASTNode::dump(indent);
  for(auto& child: mDeclarations) {
    ret += child->dump(indent + 1);
  }
  return ret;
}
std::string FunctionDeclarationNode::dump(unsigned indent) const {
  auto ret = ASTNode::dump(indent);
  ret += createIndent(indent + 1) + "Name: " + mName + "\n";
  ret += createIndent(indent + 1) + "Returns: " + mReturnType.toString() + "\n";
  ret += createIndent(indent + 1) + "Params: \n" + mParameters->dump(indent + 2);
  ret += createIndent(indent + 1) + "Body: \n" + mBody->dump(indent + 2);
  return ret;
}
FunctionDeclarationNode::FunctionDeclarationNode(std::string name,
                                                 up<ParameterListNode> params,
                                                 Datatype returnType,
                                                 up<ScopeNode> body)
: mName(std::move(name)), mParameters(std::move(params)), mReturnType(std::move(returnType)), mBody(std::move(body)) {

}

}