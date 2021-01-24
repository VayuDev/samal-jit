#pragma once

#include <vector>

namespace samal {

class ASTNode;
class DeclarationNode;
class ModuleRootNode;
class IdentifierNode;
class ExpressionNode;
class FunctionDeclarationNode;
class ScopeNode;
class BinaryExpressionNode;
class IfExpressionNode;
class FunctionCallExpressionNode;
class FunctionChainExpressionNode;
class TupleCreationNode;
class TupleAccessExpressionNode;
class AssignmentExpression;

class Compiler;
class Datatype;
class ExternalVMValue;
class VM;

}