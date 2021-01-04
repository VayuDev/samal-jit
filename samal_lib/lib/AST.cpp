#include <cassert>
#include "samal_lib/AST.hpp"
#include "easy_iterator.h"
#include "samal_lib/DatatypeCompleter.hpp"
#include "samal_lib/Compiler.hpp"

using namespace easy_iterator;

namespace samal {

static std::string createIndent(unsigned indent) {
  std::string ret;
  for(auto _: range(indent)) {
    ret += ' ';
  }
  return ret;
}

ASTNode::ASTNode(SourceCodeRef source)
: mSourceCodeRef(std::move(source)) {

}
std::string ASTNode::dump(unsigned int indent) const {
  return createIndent(indent) + getClassName() + "\n";
}
void ASTNode::throwException(const std::string &msg) const {
  throw DatatypeCompletionException(
      std::string{"In "}
      + getClassName() + ": '"
      + std::string{std::string_view{mSourceCodeRef.start, mSourceCodeRef.len}}
      + "' (" + std::to_string(mSourceCodeRef.line) + ":" + std::to_string(mSourceCodeRef.column) + ")"
      + ": " + msg);
}

ParameterListNode::ParameterListNode(SourceCodeRef source, std::vector<Parameter> params)
: ASTNode(std::move(source)), mParams(std::move(params)) {

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

ExpressionListNodeWithoutDatatypes::ExpressionListNodeWithoutDatatypes(SourceCodeRef source, std::vector<up<ExpressionNode>> params)
: ASTNode(std::move(source)), mParams(std::move(params)) {

}
std::string ExpressionListNodeWithoutDatatypes::dump(unsigned int indent) const {
  auto ret = ASTNode::dump(indent);
  for(auto& child: mParams) {
    ret += child->dump(indent + 1);
  }
  return ret;
}
void ExpressionListNodeWithoutDatatypes::completeDatatype(DatatypeCompleter &declList) {
  for(auto& child: mParams) {
    child->completeDatatype(declList);
  }
}
const std::vector<up<ExpressionNode>> &ExpressionListNodeWithoutDatatypes::getParams() const {
  return mParams;
}
ExpressionNode::ExpressionNode(SourceCodeRef source)
: ASTNode(source) {

}

AssignmentExpression::AssignmentExpression(SourceCodeRef source, up<IdentifierNode> left, up<ExpressionNode> right)
: ExpressionNode(std::move(source)), mLeft(std::move(left)), mRight(std::move(right)) {

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
void AssignmentExpression::compile(Compiler &comp) const {
  mRight->compile(comp);
  comp.assignToVariable(mLeft);
}

BinaryExpressionNode::BinaryExpressionNode(SourceCodeRef source,
                                           up<ExpressionNode> left,
                                           BinaryExpressionNode::BinaryOperator op,
                                           up<ExpressionNode> right)
: ExpressionNode(std::move(source)), mLeft(std::move(left)), mOperator(op), mRight(std::move(right)) {

}
std::optional<Datatype> BinaryExpressionNode::getDatatype() const {
  auto lhsType = mLeft->getDatatype();
  auto rhsType = mRight->getDatatype();
  if(!lhsType || !rhsType)
    return {};
  return lhsType;
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
  auto lhsType = mLeft->getDatatype();
  auto rhsType = mRight->getDatatype();
  if(lhsType != rhsType) {
    throwException("lhs=" + lhsType->toString() + " and rhs=" + rhsType->toString() + " don't match");
  }
}
void BinaryExpressionNode::compile(Compiler &comp) const {
  mLeft->compile(comp);
  mRight->compile(comp);
  auto inputsType = *mLeft->getDatatype();
  comp.binaryOperation(inputsType, mOperator);
}

LiteralNode::LiteralNode(SourceCodeRef source)
    : ExpressionNode(source) {

}

LiteralInt32Node::LiteralInt32Node(SourceCodeRef source, int32_t val)
: LiteralNode(std::move(source)), mValue(val) {

}
std::optional<Datatype> LiteralInt32Node::getDatatype() const {
  return Datatype(DatatypeCategory::i32);
}
std::string LiteralInt32Node::dump(unsigned int indent) const {
  return createIndent(indent) + getClassName() + ": " + std::to_string(mValue) + "\n";
}
void LiteralInt32Node::completeDatatype(DatatypeCompleter &) {

}
void LiteralInt32Node::compile(Compiler &comp) const {
  comp.pushPrimitiveLiteral(mValue);
}

IdentifierNode::IdentifierNode(SourceCodeRef source, std::vector<std::string> name)
: ExpressionNode(std::move(source)), mName(std::move(name)) {

}
std::optional<Datatype> IdentifierNode::getDatatype() const {
  return mDatatype ? mDatatype->first : std::optional<Datatype>{};
}
std::optional<int32_t> IdentifierNode::getId() const {
  return mDatatype ? mDatatype->second : std::optional<int32_t>{};
}
std::string IdentifierNode::dump(unsigned int indent) const {
  return
  createIndent(indent) + getClassName() + ": " + concat(mName)
  + ", type: " + (mDatatype ? mDatatype->first.toString() : "<unknown>")
  + (mDatatype ? ", id: " + std::to_string(mDatatype->second) : "") + "\n";
}
void IdentifierNode::completeDatatype(DatatypeCompleter &declList) {
  mDatatype = declList.getVariableType(mName);
}
std::string IdentifierNode::getName() const {
  return concat(mName);
}
void IdentifierNode::compile(Compiler &comp) const {
  comp.loadVariableToStack(*this);
}

TupleCreationNode::TupleCreationNode(SourceCodeRef source, up<ExpressionListNodeWithoutDatatypes> params)
: ExpressionNode(std::move(source)), mParams(std::move(params)) {

}
void TupleCreationNode::completeDatatype(DatatypeCompleter &declList) {
  mParams->completeDatatype(declList);
}
std::optional<Datatype> TupleCreationNode::getDatatype() const {
  std::vector<Datatype> paramTypes;
  for(auto& child: mParams->getParams()) {
    auto childType = child->getDatatype();
    if(!childType) {
      return {};
    }
    paramTypes.emplace_back(std::move(*childType));
  }
  return Datatype{std::move(paramTypes)};
}
std::string TupleCreationNode::dump(unsigned int indent) const {
  return ASTNode::dump(indent);
}

ListCreationNode::ListCreationNode(SourceCodeRef source, up<ExpressionListNodeWithoutDatatypes> params)
: ExpressionNode(std::move(source)), mParams(std::move(params)) {

}
ListCreationNode::ListCreationNode(SourceCodeRef source, Datatype baseType)
: ExpressionNode(std::move(source)), mBaseType(std::move(baseType)){

}
void ListCreationNode::completeDatatype(DatatypeCompleter &declList) {
  if(mParams) {
    mParams->completeDatatype(declList);
    for(auto& child: mParams->getParams()) {
      auto childType = child->getDatatype();
      assert(childType);
      if(mBaseType) {
        if(childType != *mBaseType) {
          throwException("Not all elements in the created list have the same type; previous children had the type "
                             + mBaseType->toString() + ", but one has the type " + childType->toString());
        }
      } else {
        mBaseType = childType;
      }
    }
  }
  if(!mBaseType) {
    throwException("Can't determine the type of this list. Hint: Try something like [:i32] to create an empty list.");
  }
}
std::optional<Datatype> ListCreationNode::getDatatype() const {
  if(!mBaseType)
    return {};
  return Datatype::createListType(*mBaseType);
}
std::string ListCreationNode::dump(unsigned int indent) const {
  auto ret = ASTNode::dump(indent);
  if(mParams) {
    ret += mParams->dump(indent + 1);
  } else if(mBaseType) {
    ret += createIndent(indent + 1) + "Base type: " + mBaseType->toString() + "\n";
  }
  return ret;
}

ScopeNode::ScopeNode(SourceCodeRef source, std::vector<up<ExpressionNode>> expressions)
: ExpressionNode(std::move(source)), mExpressions(std::move(expressions)) {

}
std::optional<Datatype> ScopeNode::getDatatype() const {
  if(mExpressions.empty()) {
    return Datatype::createEmptyTuple();
  }
  return mExpressions.at(mExpressions.size() - 1)->getDatatype();
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
void ScopeNode::compile(Compiler &comp) const {
  auto duration = comp.enterScope(*getDatatype());
  size_t i = 0;
  for(auto& expr: mExpressions) {
    expr->compile(comp);
    // Pop the values from all lines at the end except for the last line.
    // This is because the last line is the return value.
    if(i < mExpressions.size() - 1) {
      comp.popUnusedValueAtEndOfScope(*expr->getDatatype());
    }
    i += 1;
  }
}

IfExpressionNode::IfExpressionNode(SourceCodeRef source, IfExpressionChildList children, up<ScopeNode> elseBody)
: ExpressionNode(std::move(source)), mChildren(std::move(children)), mElseBody(std::move(elseBody)) {

}
std::optional<Datatype> IfExpressionNode::getDatatype() const {
  return mChildren.front().second->getDatatype();
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
  std::optional<Datatype> type;
  for(auto& child: mChildren) {
    child.first->completeDatatype(declList);
    child.second->completeDatatype(declList);
    // ensure that every possible branch has the same type
    auto childType = child.second->getDatatype();
    assert(childType);
    if(type) {
      if(*childType != *type) {
        throwException("Not all branches of this if expression return the same value. "
                       "Previous branches return " + type->toString() + ", but one returns " + childType->toString());
      }
    } else {
      type = std::move(*childType);
    }
  }
  if(mElseBody) {
    mElseBody->completeDatatype(declList);
    auto elseBodyType = mElseBody->getDatatype();
    assert(elseBodyType);
    if(elseBodyType != *type) {
      throwException("The else branch of this if expression returns a value different from the other branches. "
                     "Previous branches return " + type->toString() + ", but the else branch returns " + elseBodyType->toString());
    }
  }
}

FunctionCallExpressionNode::FunctionCallExpressionNode(SourceCodeRef source,
                                                       up<ExpressionNode> name,
                                                       up<ExpressionListNodeWithoutDatatypes> params)
: ExpressionNode(std::move(source)), mName(std::move(name)), mParams(std::move(params)) {

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
  auto identifierType = mName->getDatatype();
  assert(identifierType);
  if(identifierType->getCategory() != DatatypeCategory::function) {
    throwException("Calling non-function type '" + identifierType->toString() + "'");
  }
  auto& functionType = identifierType->getFunctionTypeInfo();
  if(functionType.second.size() != mParams->getParams().size()) {
    throwException("Function " + identifierType->toString() + " expects " + std::to_string(functionType.second.size())
      + " arguments, but " + std::to_string(mParams->getParams().size()) + " have been passed");
  }
  size_t i = 0;
  for(size_t i = 0; i < functionType.second.size(); ++i) {
    auto passedType = mParams->getParams().at(i)->getDatatype();
    assert(passedType);
    auto& expectedType = functionType.second.at(i);
    if(expectedType != passedType) {
      throwException("Function " + identifierType->toString() + " got passed invalid argument types; at position "
        + std::to_string(i) + " we expected a '" + expectedType.toString() + "', but got passed a '" + passedType->toString() + "'");
    }
  }
}
void FunctionCallExpressionNode::compile(Compiler &comp) const {
  mName->compile(comp);
  size_t sizeOfArguments = 0;
  auto functionType = mName->getDatatype();
  assert(functionType);
  auto returnType = functionType->getFunctionTypeInfo().first;
  for(auto& param: mParams->getParams()) {
    param->compile(comp);
    sizeOfArguments += param->getDatatype()->getSizeOnStack();
  }
  comp.performFunctionCall(sizeOfArguments, returnType->getSizeOnStack());
}

ListAccessExpressionNode::ListAccessExpressionNode(SourceCodeRef source,
                                                   up<ExpressionNode> name,
                                                   up<ExpressionNode> index)
: ExpressionNode(std::move(source)), mName(std::move(name)), mIndex(std::move(index)) {

}
std::optional<Datatype> ListAccessExpressionNode::getDatatype() const {
  if(!mName->getDatatype())
    return {};
  return mName->getDatatype()->getListInfo();
}
void ListAccessExpressionNode::completeDatatype(DatatypeCompleter &declList) {
  mName->completeDatatype(declList);
  mIndex->completeDatatype(declList);
  auto indexType = mIndex->getDatatype();
  assert(indexType);
  if(!indexType->isInteger()) {
    throwException("List access index must be numeric, e.g. i32; index type is " + indexType->toString());
  }
}
std::string ListAccessExpressionNode::dump(unsigned int indent) const {
  auto ret = ASTNode::dump(indent);
  ret += createIndent(indent + 1) + "Name:\n";
  ret += mName->dump(indent + 2);
  ret += createIndent(indent + 1) + "Index:\n";
  ret += mIndex->dump(indent + 2);
  return ret;
}

DeclarationNode::DeclarationNode(SourceCodeRef source)
: ASTNode(std::move(source)) {

}

ModuleRootNode::ModuleRootNode(SourceCodeRef source, std::vector<up<DeclarationNode>> &&declarations)
: ASTNode(std::move(source)), mDeclarations(std::move(declarations)) {
}
std::string ModuleRootNode::dump(unsigned indent) const {
  auto ret = ASTNode::dump(indent);
  ret += createIndent(indent + 1) + "Name: " + mName + "\n";
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
  auto scope = declList.openScope(mName);
  for(auto& child: mDeclarations) {
    child->completeDatatype(declList);
  }
}
void ModuleRootNode::declareShallow(DatatypeCompleter &completer) const {
  auto scope = completer.openScope();
  for(auto& child: mDeclarations) {
    child->declareShallow(completer);
  }
  completer.saveModule(mName);
}
void ModuleRootNode::setModuleName(std::string name) {
  mName = std::move(name);
}
void ModuleRootNode::compile(Compiler &comp) const {
  for(auto& decl: mDeclarations) {
    decl->compile(comp);
  }
}

std::string FunctionDeclarationNode::dump(unsigned indent) const {
  auto ret = ASTNode::dump(indent);
  ret += createIndent(indent + 1) + "Name:\n" + mName->dump(indent + 2);
  ret += createIndent(indent + 1) + "Returns: " + mReturnType.toString() + "\n";
  ret += createIndent(indent + 1) + "Params: \n" + mParameters->dump(indent + 2);
  ret += createIndent(indent + 1) + "Body: \n" + mBody->dump(indent + 2);
  return ret;
}
FunctionDeclarationNode::FunctionDeclarationNode(SourceCodeRef source,
                                                 up<IdentifierNode> name,
                                                 up<ParameterListNode> params,
                                                 Datatype returnType,
                                                 up<ScopeNode> body)
: DeclarationNode(std::move(source)), mName(std::move(name)), mParameters(std::move(params)), mReturnType(std::move(returnType)), mBody(std::move(body)) {

}
void FunctionDeclarationNode::completeDatatype(DatatypeCompleter &declList) {
  auto scope = declList.openScope();
  mName->completeDatatype(declList);
  mParameters->completeDatatype(declList);
  mBody->completeDatatype(declList);
  auto bodyType = mBody->getDatatype();
  assert(bodyType);
  if(bodyType != mReturnType) {
    throwException("This function's declared return type (" + mReturnType.toString() + ") and actual return type (" + bodyType->toString() + ") don't match");
  }
}
void FunctionDeclarationNode::declareShallow(DatatypeCompleter &completer) const {
  // TODO also set params etc.
  std::vector<Datatype> paramTypes;
  for(auto& p: mParameters->getParams()) {
    paramTypes.emplace_back(p.type);
  }
  try {
    completer.declareVariableNonOverrideable(mName->getName(), Datatype{mReturnType, std::move(paramTypes)});
  } catch(std::runtime_error& e) {
    throwException(e.what());
  }
}
void FunctionDeclarationNode::compile(Compiler &comp) const {
  auto duration = comp.enterFunction(mName);
  size_t offsetFromTop = 0;
  for(auto& param: reverse(mParameters->getParams())) {
    comp.setVariableLocation(param.name, offsetFromTop);
    offsetFromTop += param.type.getSizeOnStack();
  }
  mBody->compile(comp);
}
}