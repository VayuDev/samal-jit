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
  return ret;
}
FunctionDeclarationNode::FunctionDeclarationNode(std::string name,
                                                 up<ParameterListNode> params,
                                                 Datatype returnType,
                                                 up<ScopeNode> body)
: mName(std::move(name)), mParameters(std::move(params)), mReturnType(std::move(returnType)), mBody(std::move(body)) {

}
}