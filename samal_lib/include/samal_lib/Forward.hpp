#pragma once

#include <vector>
#include <map>
#include <string>

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
class TailCallSelfStatementNode;
class MatchExpression;
class MoveToHeapExpression;
class MoveToStackExpression;
class CallableDeclarationNode;

struct Parameter;
using StructField = Parameter;
struct EnumField;

class Compiler;
class Datatype;
class ExternalVMValue;
class VM;
class Stack;
struct Program;
class VariableSearcher;
class Parser;
struct VMParameters;

enum class TemplateParamOrUserType;

using UndeterminedIdentifierReplacementMap = std::map<std::string, std::pair<Datatype, TemplateParamOrUserType>>;

}