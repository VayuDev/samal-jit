#include "samal_lib/AST.hpp"
#include "samal_lib/Compiler.hpp"
#include <cassert>

namespace samal {

static std::string createIndent(unsigned indent) {
    std::string ret;
    for(unsigned i = 0; i < indent; ++i) {
        ret += ' ';
    }
    return ret;
}

static std::string dumpExpressionVector(unsigned indent, const std::vector<up<ExpressionNode>>& expression) {
    std::string ret;
    for(auto& expr : expression) {
        ret += expr->dump(indent);
    }
    return ret;
}

static std::string dumpParameterVector(unsigned indent, const std::vector<Parameter>& parameters) {
    std::string ret;
    for(auto& param : parameters) {
        ret += createIndent(indent + 1) + "Name:\n";
        ret += param.name->dump(indent + 2);
        ret += createIndent(indent + 1) + "Type: " + param.type.toString() + "\n";
    }
    return ret;
}

ASTNode::ASTNode(SourceCodeRef source)
: mSourceCodeRef(std::move(source)) {
}
std::string ASTNode::dump(unsigned int indent) const {
    return createIndent(indent) + getClassName() + "\n";
}
void ASTNode::throwException(const std::string& msg) const {
    throw DatatypeCompletionException(
        std::string{ "In " }
        + getClassName() + ": '"
        + std::string{ std::string_view{ mSourceCodeRef.start, mSourceCodeRef.len } }
        + "' (" + std::to_string(mSourceCodeRef.line) + ":" + std::to_string(mSourceCodeRef.column) + ")"
        + ": " + msg);
}

ExpressionNode::ExpressionNode(SourceCodeRef source)
: ASTNode(source) {
}

AssignmentExpression::AssignmentExpression(SourceCodeRef source, up<IdentifierNode> left, up<ExpressionNode> right)
: ExpressionNode(std::move(source)), mLeft(std::move(left)), mRight(std::move(right)) {
}
std::string AssignmentExpression::dump(unsigned int indent) const {
    auto ret = ASTNode::dump(indent);
    ret += createIndent(indent + 1) + "Left:\n" + mLeft->dump(indent + 2);
    ret += createIndent(indent + 1) + "Right:\n" + mRight->dump(indent + 2);
    return ret;
}
Datatype AssignmentExpression::compile(Compiler& comp) const {
    return Datatype{ DatatypeCategory::invalid };
}
const up<IdentifierNode>& AssignmentExpression::getLeft() const {
    return mLeft;
}
const up<ExpressionNode>& AssignmentExpression::getRight() const {
    return mRight;
}

BinaryExpressionNode::BinaryExpressionNode(SourceCodeRef source,
    up<ExpressionNode> left,
    BinaryExpressionNode::BinaryOperator op,
    up<ExpressionNode> right)
: ExpressionNode(std::move(source)), mLeft(std::move(left)), mOperator(op), mRight(std::move(right)) {
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
Datatype BinaryExpressionNode::compile(Compiler& comp) const {
    return Datatype{ DatatypeCategory::invalid };
}

LiteralNode::LiteralNode(SourceCodeRef source)
: ExpressionNode(source) {
}

LiteralInt32Node::LiteralInt32Node(SourceCodeRef source, int32_t val)
: LiteralNode(std::move(source)), mValue(val) {
}
std::string LiteralInt32Node::dump(unsigned int indent) const {
    return createIndent(indent) + getClassName() + ": " + std::to_string(mValue) + "\n";
}
Datatype LiteralInt32Node::compile(Compiler& comp) const {
    return Datatype{ DatatypeCategory::invalid };
}

LiteralInt64Node::LiteralInt64Node(SourceCodeRef source, int64_t val)
: LiteralNode(std::move(source)), mValue(val) {
}
Datatype LiteralInt64Node::compile(Compiler& comp) const {
    return Datatype{ DatatypeCategory::invalid };
}
std::string LiteralInt64Node::dump(unsigned int indent) const {
    return createIndent(indent) + getClassName() + ": " + std::to_string(mValue) + "\n";
}

LiteralBoolNode::LiteralBoolNode(SourceCodeRef source, bool val)
: LiteralNode(std::move(source)), mValue(val) {
}
Datatype LiteralBoolNode::compile(Compiler& comp) const {
    return Datatype{ DatatypeCategory::invalid };
}
std::string LiteralBoolNode::dump(unsigned int indent) const {
    return createIndent(indent) + getClassName() + ": " + std::to_string(mValue) + "\n";
}

IdentifierNode::IdentifierNode(SourceCodeRef source, std::vector<std::string> name, std::vector<Datatype> templateParameters)
: ExpressionNode(std::move(source)), mName(std::move(name)), mTemplateParameters(std::move(templateParameters)) {
}
std::string IdentifierNode::dump(unsigned int indent) const {
    std::string ret = createIndent(indent) + getClassName() + ": " + concat(mName);
    if(!mTemplateParameters.empty()) {
        ret += ", template parameters: ";
        for(auto& param : mTemplateParameters) {
            ret += param.toString();
            ret += ",";
        }
    }
    return ret + "\n";
}
std::string IdentifierNode::getName() const {
    return concat(mName);
}
std::vector<std::string> IdentifierNode::getNameSplit() const {
    return mName;
}
Datatype IdentifierNode::compile(Compiler& comp) const {
    return Datatype{ DatatypeCategory::invalid };
}
const std::vector<Datatype>& IdentifierNode::getTemplateParameters() const {
    return mTemplateParameters;
}

TupleCreationNode::TupleCreationNode(SourceCodeRef source, std::vector<up<ExpressionNode>> params)
: ExpressionNode(std::move(source)), mParams(std::move(params)) {
}
std::string TupleCreationNode::dump(unsigned int indent) const {
    return ASTNode::dump(indent);
}
Datatype TupleCreationNode::compile(Compiler& comp) const {
    return Datatype{ DatatypeCategory::invalid };
}

ListCreationNode::ListCreationNode(SourceCodeRef source, std::vector<up<ExpressionNode>> params)
: ExpressionNode(std::move(source)), mParams(std::move(params)) {
}
ListCreationNode::ListCreationNode(SourceCodeRef source, Datatype baseType)
: ExpressionNode(std::move(source)), mBaseType(std::move(baseType)) {
}
std::string ListCreationNode::dump(unsigned int indent) const {
    auto ret = ASTNode::dump(indent);
    if(!mParams.empty()) {
        for(auto& param : mParams)
            ret += param->dump(indent + 1);
    } else if(mBaseType) {
        ret += createIndent(indent + 1) + "Base type: " + mBaseType->toString() + "\n";
    }
    return ret;
}

ScopeNode::ScopeNode(SourceCodeRef source, std::vector<up<ExpressionNode>> expressions)
: ExpressionNode(std::move(source)), mExpressions(std::move(expressions)) {
}
std::string ScopeNode::dump(unsigned int indent) const {
    auto ret = ASTNode::dump(indent);
    for(auto& child : mExpressions) {
        ret += child->dump(indent + 1);
    }
    return ret;
}
Datatype ScopeNode::compile(Compiler& comp) const {
    return Datatype{ DatatypeCategory::invalid };
}

IfExpressionNode::IfExpressionNode(SourceCodeRef source, IfExpressionChildList children, up<ScopeNode> elseBody)
: ExpressionNode(std::move(source)), mChildren(std::move(children)), mElseBody(std::move(elseBody)) {
}
std::string IfExpressionNode::dump(unsigned int indent) const {
    auto ret = ASTNode::dump(indent);
    for(auto& child : mChildren) {
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
Datatype IfExpressionNode::compile(Compiler& comp) const {
    return Datatype{ DatatypeCategory::invalid };
}

FunctionCallExpressionNode::FunctionCallExpressionNode(SourceCodeRef source,
    up<ExpressionNode> name,
    std::vector<up<ExpressionNode>> params)
: ExpressionNode(std::move(source)), mName(std::move(name)), mParams(std::move(params)) {
}
std::string FunctionCallExpressionNode::dump(unsigned int indent) const {
    auto ret = ASTNode::dump(indent);
    ret += mName->dump(indent + 1);
    ret += dumpExpressionVector(indent + 1, mParams);
    return ret;
}
Datatype FunctionCallExpressionNode::compile(Compiler& comp) const {
    return Datatype{ DatatypeCategory::invalid };
}
void FunctionCallExpressionNode::prependChainedParameter(Datatype chainedParamType) {
    mPrependedChainedParameterType = std::move(chainedParamType);
}

FunctionChainExpressionNode::FunctionChainExpressionNode(SourceCodeRef source, up<ExpressionNode> initialValue, up<FunctionCallExpressionNode> functionCall)
: ExpressionNode(std::move(source)), mInitialValue(std::move(initialValue)), mFunctionCall(std::move(functionCall)) {
}
Datatype FunctionChainExpressionNode::compile(Compiler& comp) const {
    return Datatype{ DatatypeCategory::invalid };
}
std::string FunctionChainExpressionNode::dump(unsigned int indent) const {
    auto ret = ASTNode::dump(indent);
    ret += createIndent(indent + 1) + "Initial value:\n";
    ret += mInitialValue->dump(indent + 2);
    ret += createIndent(indent + 1) + "Function:\n";
    ret += mFunctionCall->dump(indent + 2);
    return ret;
}

ListAccessExpressionNode::ListAccessExpressionNode(SourceCodeRef source,
    up<ExpressionNode> name,
    up<ExpressionNode> index)
: ExpressionNode(std::move(source)), mName(std::move(name)), mIndex(std::move(index)) {
}
std::string ListAccessExpressionNode::dump(unsigned int indent) const {
    auto ret = ASTNode::dump(indent);
    ret += createIndent(indent + 1) + "Name:\n";
    ret += mName->dump(indent + 2);
    ret += createIndent(indent + 1) + "Index:\n";
    ret += mIndex->dump(indent + 2);
    return ret;
}

TupleAccessExpressionNode::TupleAccessExpressionNode(SourceCodeRef source, up<ExpressionNode> name, uint32_t index)
: ExpressionNode(std::move(source)), mName(std::move(name)), mIndex(index) {
}
Datatype TupleAccessExpressionNode::compile(Compiler& comp) const {
    return Datatype{ DatatypeCategory::invalid };
}
std::string TupleAccessExpressionNode::dump(unsigned int indent) const {
    auto ret = ASTNode::dump(indent);
    ret += createIndent(indent + 1) + "Name:\n";
    ret += mName->dump(indent + 2);
    ret += createIndent(indent + 1) + "Index:";
    ret += std::to_string(mIndex) + "\n";
    return ret;
}

DeclarationNode::DeclarationNode(SourceCodeRef source)
: ASTNode(std::move(source)) {
}

ModuleRootNode::ModuleRootNode(SourceCodeRef source, std::vector<up<DeclarationNode>>&& declarations)
: ASTNode(std::move(source)), mDeclarations(std::move(declarations)) {
}
std::string ModuleRootNode::dump(unsigned indent) const {
    auto ret = ASTNode::dump(indent);
    ret += createIndent(indent + 1) + "Name: " + mName + "\n";
    for(auto& child : mDeclarations) {
        ret += child->dump(indent + 1);
    }
    return ret;
}
std::vector<DeclarationNode*> ModuleRootNode::createDeclarationList() {
    std::vector<DeclarationNode*> ret;
    ret.reserve(mDeclarations.size());
    for(auto& child : mDeclarations) {
        ret.emplace_back(child.get());
    }
    return ret;
}
void ModuleRootNode::setModuleName(std::string name) {
    mName = std::move(name);
}
Datatype ModuleRootNode::compile(Compiler& comp) const {
    return Datatype{ DatatypeCategory::invalid };
}
const std::vector<up<DeclarationNode>>& ModuleRootNode::getDeclarations() const {
    return mDeclarations;
}
const std::string& ModuleRootNode::getModuleName() const {
    return mName;
}

std::string FunctionDeclarationNode::dump(unsigned indent) const {
    auto ret = ASTNode::dump(indent);
    ret += createIndent(indent + 1) + "Name:\n" + mName->dump(indent + 2);
    ret += createIndent(indent + 1) + "Returns: " + mReturnType.toString() + "\n";
    ret += createIndent(indent + 1) + "Params: \n" + dumpParameterVector(indent + 2, mParameters);
    ret += createIndent(indent + 1) + "Body: \n" + mBody->dump(indent + 2);
    return ret;
}
FunctionDeclarationNode::FunctionDeclarationNode(SourceCodeRef source,
    up<IdentifierNode> name,
    std::vector<Parameter> params,
    Datatype returnType,
    up<ScopeNode> body)
: DeclarationNode(std::move(source)), mName(std::move(name)), mParameters(std::move(params)), mReturnType(std::move(returnType)), mBody(std::move(body)) {
}
Datatype FunctionDeclarationNode::compile(Compiler& comp) const {
    return Datatype{ DatatypeCategory::invalid };
}
bool FunctionDeclarationNode::hasTemplateParameters() const {
    for(auto& param : mParameters) {
        if(param.type.hasUndeterminedTemplateTypes())
            return true;
    }
    return mReturnType.hasUndeterminedTemplateTypes();
}
std::vector<Datatype> FunctionDeclarationNode::getTemplateParameterVector() const {
    return mName->getTemplateParameters();
}
const IdentifierNode* FunctionDeclarationNode::getIdentifier() const {
    return mName.get();
}
}