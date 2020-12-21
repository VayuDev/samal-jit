#include <cassert>
#include "samal_lib/AST.hpp"
#include "easy_iterator.h"
#include "samal_lib/DatatypeCompleter.hpp"

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
void ParameterListNode::completeDatatype(DatatypeCompleter &declList) {
  for(auto& param: mParams) {
    declList.declareVariable(param.name->getName(), param.type);
    param.name->completeDatatype(declList);
  }
}
const std::vector<ParameterListNode::Parameter>& ParameterListNode::getParams() {
  return mParams;
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
void ParameterListNodeWithoutDatatypes::completeDatatype(DatatypeCompleter &declList) {
  for(auto& child: mParams) {
    child->completeDatatype(declList);
  }
}

AssignmentExpression::AssignmentExpression(up<IdentifierNode> left, up<ExpressionNode> right)
: mLeft(std::move(left)), mRight(std::move(right)) {

}
std::optional<Datatype> AssignmentExpression::getDatatype() const {
  return mRight->getDatatype();
}
std::string AssignmentExpression::dump(unsigned int indent) const {
  auto ret = ASTNode::dump(indent);
  ret += createIndent(indent + 1) + "Left:\n" + mLeft->dump(indent + 2);
  ret += createIndent(indent + 1) + "Right:\n" + mRight->dump(indent + 2);
  return ret;
}
void AssignmentExpression::completeDatatype(DatatypeCompleter &declList) {
  mRight->completeDatatype(declList);
  auto rightType = mRight->getDatatype();
  assert(rightType);
  declList.declareVariable(mLeft->getName(), *rightType);
  mLeft->completeDatatype(declList);
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
void BinaryExpressionNode::completeDatatype(DatatypeCompleter &declList) {
  mLeft->completeDatatype(declList);
  mRight->completeDatatype(declList);
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
void LiteralInt32Node::completeDatatype(DatatypeCompleter &declList) {

}

IdentifierNode::IdentifierNode(std::string name)
: mName(std::move(name)) {

}
std::optional<Datatype> IdentifierNode::getDatatype() const {
  return mDatatype ? mDatatype->first : std::optional<Datatype>{};
}
std::string IdentifierNode::dump(unsigned int indent) const {
  return
  createIndent(indent) + getClassName() + ": " + mName
  + ", type: " + (mDatatype ? mDatatype->first.toString() : "<unknown>")
  + (mDatatype ? ", id: " + std::to_string(mDatatype->second) : "") + "\n";
}
void IdentifierNode::completeDatatype(DatatypeCompleter &declList) {
  mDatatype = declList.getVariableType(mName);
}
const std::string &IdentifierNode::getName() const {
  return mName;
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
void ScopeNode::completeDatatype(DatatypeCompleter &declList) {
  auto scope = declList.openScope();
  for(auto& child: mExpressions) {
    child->completeDatatype(declList);
  }
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
void IfExpressionNode::completeDatatype(DatatypeCompleter &declList) {
  for(auto& child: mChildren) {
    child.first->completeDatatype(declList);
    child.second->completeDatatype(declList);
  }
  if(mElseBody)
    mElseBody->completeDatatype(declList);
}

FunctionCallExpressionNode::FunctionCallExpressionNode(up<ExpressionNode> name,
                                                       up<ParameterListNodeWithoutDatatypes> params)
: mName(std::move(name)), mParams(std::move(params)) {

}
std::optional<Datatype> FunctionCallExpressionNode::getDatatype() const {
  return *mName->getDatatype()->getFunctionTypeInfo().first;
}
std::string FunctionCallExpressionNode::dump(unsigned int indent) const {
  auto ret = ASTNode::dump(indent);
  ret += mName->dump(indent + 1);
  ret += mParams->dump(indent + 1);
  return ret;
}
void FunctionCallExpressionNode::completeDatatype(DatatypeCompleter &declList) {
  mName->completeDatatype(declList);
  mParams->completeDatatype(declList);
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
std::vector<DeclarationNode*> ModuleRootNode::createDeclarationList() {
  std::vector<DeclarationNode*> ret;
  ret.reserve(mDeclarations.size());
  for(auto& child: mDeclarations) {
    ret.emplace_back(child.get());
  }
  return ret;
}
void ModuleRootNode::completeDatatype(DatatypeCompleter &declList) {
  auto scope = declList.openScope();
  for(auto& child: mDeclarations) {
    child->completeDatatype(declList);
  }
}
void ModuleRootNode::declareShallow(DatatypeCompleter &completer) const {
  auto scope = completer.openScope();
  for(auto& child: mDeclarations) {
    child->declareShallow(completer);
  }
  completer.saveModule("This is a placeholder for the actual module name");
}
std::string FunctionDeclarationNode::dump(unsigned indent) const {
  auto ret = ASTNode::dump(indent);
  ret += createIndent(indent + 1) + "Name:\n" + mName->dump(indent + 2);
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
void FunctionDeclarationNode::completeDatatype(DatatypeCompleter &declList) {
  auto scope = declList.openScope();
  mName->completeDatatype(declList);
  mParameters->completeDatatype(declList);
  mBody->completeDatatype(declList);
}
void FunctionDeclarationNode::declareShallow(DatatypeCompleter &completer) const {
  // TODO also set params etc.
  std::vector<Datatype> paramTypes;
  for(auto& p: mParameters->getParams()) {
    paramTypes.emplace_back(p.type);
  }
  completer.declareVariableNonOverrideable(mName->getName(), Datatype{mReturnType, std::move(paramTypes)});
}

}