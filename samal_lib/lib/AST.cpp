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
    ret += child.name->dump(indent + 1);
    ret += createIndent(indent + 1) + "Type: " + child.type.toString() + "\n";
  }
  return ret;
}

ParameterListNodeWithoutDatatypes::ParameterListNodeWithoutDatatypes(std::vector<up<ExpressionNode>> params)
: mParams(std::move(params)) {

}
std::string ParameterListNodeWithoutDatatypes::dump(unsigned int indent) const {
  auto ret = ASTNode::dump(indent);
  for(auto& child: mParams) {
    ret += child->dump(indent + 1);
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

    case BinaryOperator::LOGICAL_AND:
      ret += "&&";
      break;
    case BinaryOperator::LOGICAL_OR:
      ret += "||";
      break;
    case BinaryOperator::LOGICAL_EQUALS:
      ret += "==";
      break;
    case BinaryOperator::LOGICAL_NOT_EQUALS:
      ret += "!=";
      break;
    case BinaryOperator::COMPARISON_LESS_THAN:
      ret += "<";
      break;
    case BinaryOperator::COMPARISON_LESS_EQUAL_THAN:
      ret += "<=";
      break;
    case BinaryOperator::COMPARISON_MORE_THAN:
      ret += ">";
      break;
    case BinaryOperator::COMPARISON_MORE_EQUAL_THAN:
      ret += ">=";
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

IdentifierNode::IdentifierNode(std::string name)
: mName(std::move(name)) {

}
std::optional<Datatype> IdentifierNode::getDatatype() const {
  return std::optional<Datatype>();
}
std::string IdentifierNode::dump(unsigned int indent) const {
  return createIndent(indent) + getClassName() + ": " + mName + "\n";
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

IfExpressionNode::IfExpressionNode(IfExpressionChildList children, up<ScopeNode> elseBody)
: mChildren(std::move(children)), mElseBody(std::move(elseBody)) {

}
std::optional<Datatype> IfExpressionNode::getDatatype() const {
  return std::optional<Datatype>();
}
std::string IfExpressionNode::dump(unsigned int indent) const {
  auto ret = ASTNode::dump(indent);
  for(auto& child: mChildren) {
    ret += createIndent(indent + 1) + "Condition:\n" + child.first->dump(indent + 2);
    ret += createIndent(indent + 1) + "Body:\n" + child.second->dump(indent + 2);
  }
  ret += createIndent(indent + 1) + "Else:\n";
  if(mElseBody) {
    ret += mElseBody->dump(indent + 2);
  } else {
    ret += createIndent(indent + 2) + "nullptr\n";
  }
  return ret;
}

FunctionCallExpressionNode::FunctionCallExpressionNode(up<IdentifierNode> name,
                                                       up<ParameterListNodeWithoutDatatypes> params)
: mName(std::move(name)), mParams(std::move(params)) {

}
std::optional<Datatype> FunctionCallExpressionNode::getDatatype() const {
  return std::optional<Datatype>();
}
std::string FunctionCallExpressionNode::dump(unsigned int indent) const {
  auto ret = ASTNode::dump(indent);
  ret += mName->dump(indent + 1);
  ret += mParams->dump(indent + 1);
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
  ret += createIndent(indent + 1) + "Name:\n" + mName->dump(indent + 1);
  ret += createIndent(indent + 1) + "Returns: " + mReturnType.toString() + "\n";
  ret += createIndent(indent + 1) + "Params: \n" + mParameters->dump(indent + 2);
  ret += createIndent(indent + 1) + "Body: \n" + mBody->dump(indent + 2);
  return ret;
}
FunctionDeclarationNode::FunctionDeclarationNode(up<IdentifierNode> name,
                                                 up<ParameterListNode> params,
                                                 Datatype returnType,
                                                 up<ScopeNode> body)
: mName(std::move(name)), mParameters(std::move(params)), mReturnType(std::move(returnType)), mBody(std::move(body)) {

}

}