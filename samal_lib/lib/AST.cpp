#include "samal_lib/AST.hpp"

namespace samal {

ModuleRootNode::ModuleRootNode(std::vector<up<DeclarationNode>> &&declarations)
: mDeclarations(std::move(declarations)) {
}
}