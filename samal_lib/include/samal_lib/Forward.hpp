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
class LambdaCreationNode;
class StackInformationTree;
class CallableDeclarationNode;
class NativeFunctionDeclarationNode;
class ListCreationNode;
class ListPropertyAccessExpression;
class StructCreationNode;
class StructFieldAccessExpression;

struct Parameter;

class Compiler;
class Datatype;
class ExternalVMValue;
class VM;
class Stack;
struct Program;
class VariableSearcher;
class Parser;

}