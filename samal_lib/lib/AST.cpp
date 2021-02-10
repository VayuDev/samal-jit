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
    throw CompilationException(
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
    return comp.compileAssignmentExpression(*this);
}
const up<IdentifierNode>& AssignmentExpression::getLeft() const {
    return mLeft;
}
const up<ExpressionNode>& AssignmentExpression::getRight() const {
    return mRight;
}
void AssignmentExpression::findUsedVariables(VariableSearcher& searcher) const {
    mLeft->findUsedVariables(searcher);
    mRight->findUsedVariables(searcher);
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
    return comp.compileBinaryExpression(*this);
}
void BinaryExpressionNode::findUsedVariables(VariableSearcher& searcher) const {
    mLeft->findUsedVariables(searcher);
    mRight->findUsedVariables(searcher);
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
    return comp.compileLiteralI32(mValue);
}
void LiteralInt32Node::findUsedVariables(VariableSearcher&) const {
}

LiteralInt64Node::LiteralInt64Node(SourceCodeRef source, int64_t val)
: LiteralNode(std::move(source)), mValue(val) {
}
Datatype LiteralInt64Node::compile(Compiler& comp) const {
    return comp.compileLiteralI64(mValue);
}
std::string LiteralInt64Node::dump(unsigned int indent) const {
    return createIndent(indent) + getClassName() + ": " + std::to_string(mValue) + "\n";
}
void LiteralInt64Node::findUsedVariables(VariableSearcher&) const {
}

LiteralBoolNode::LiteralBoolNode(SourceCodeRef source, bool val)
: LiteralNode(std::move(source)), mValue(val) {
}
Datatype LiteralBoolNode::compile(Compiler& comp) const {
    return comp.compileLiteralBool(mValue);
}
std::string LiteralBoolNode::dump(unsigned int indent) const {
    return createIndent(indent) + getClassName() + ": " + std::to_string(mValue) + "\n";
}
void LiteralBoolNode::findUsedVariables(VariableSearcher&) const {
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
    return comp.compileIdentifierLoad(*this);
}
const std::vector<Datatype>& IdentifierNode::getTemplateParameters() const {
    return mTemplateParameters;
}
void IdentifierNode::findUsedVariables(VariableSearcher& searcher) const {
    searcher.identifierFound(*this);
}

TupleCreationNode::TupleCreationNode(SourceCodeRef source, std::vector<up<ExpressionNode>> params)
: ExpressionNode(std::move(source)), mParams(std::move(params)) {
}
std::string TupleCreationNode::dump(unsigned int indent) const {
    return ASTNode::dump(indent);
}
Datatype TupleCreationNode::compile(Compiler& comp) const {
    return comp.compileTupleCreationExpression(*this);
}
void TupleCreationNode::findUsedVariables(VariableSearcher& searcher) const {
    for(auto& param : mParams) {
        param->findUsedVariables(searcher);
    }
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
void ListCreationNode::findUsedVariables(VariableSearcher& searcher) const {
    for(auto& param : mParams) {
        param->findUsedVariables(searcher);
    }
}
Datatype ListCreationNode::compile(Compiler& comp) const {
    return comp.compileListCreation(*this);
}

std::string StructCreationNode::dump(unsigned int indent) const {
    return ASTNode::dump(indent);
}
StructCreationNode::StructCreationNode(SourceCodeRef source, Datatype structType, std::vector<StructCreationParameter> params)
: ExpressionNode(std::move(source)), mStructType(std::move(structType)), mParams(std::move(params)) {
}
Datatype StructCreationNode::compile(Compiler& comp) const {
    return comp.compileStructCreation(*this);
}
void StructCreationNode::findUsedVariables(VariableSearcher& searcher) const {
    for(auto& p: mParams) {
        p.value->findUsedVariables(searcher);
    }
}

LambdaCreationNode::LambdaCreationNode(SourceCodeRef source, std::vector<Parameter> parameters, Datatype returnType, up<ScopeNode> body)
: ExpressionNode(std::move(source)), mReturnType(std::move(returnType)), mParameters(std::move(parameters)), mBody(std::move(body)) {
}
Datatype LambdaCreationNode::compile(Compiler& comp) const {
    return comp.compileLambdaCreationExpression(*this);
}
std::string LambdaCreationNode::dump(unsigned int indent) const {
    auto ret = ASTNode::dump(indent);
    ret += createIndent(indent + 1) + "Returns: " + mReturnType.toString() + "\n";
    ret += createIndent(indent + 1) + "Params: \n" + dumpParameterVector(indent + 2, mParameters);
    ret += createIndent(indent + 1) + "Body: \n" + mBody->dump(indent + 2);
    return ret;
}
void LambdaCreationNode::findUsedVariables(VariableSearcher& searcher) const {
    for(auto& param : mParameters) {
        param.name->findUsedVariables(searcher);
    }
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
    return comp.compileScope(*this);
}
void ScopeNode::findUsedVariables(VariableSearcher& searcher) const {
    for(auto& child : mExpressions) {
        child->findUsedVariables(searcher);
    }
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
    return comp.compileIfExpression(*this);
}
void IfExpressionNode::findUsedVariables(VariableSearcher& searcher) const {
    for(auto& child : mChildren) {
        child.first->findUsedVariables(searcher);
        child.second->findUsedVariables(searcher);
    }
    if(mElseBody) {
        mElseBody->findUsedVariables(searcher);
    }
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
    return comp.compileFunctionCall(*this);
}
void FunctionCallExpressionNode::findUsedVariables(VariableSearcher& searcher) const {
    mName->findUsedVariables(searcher);
    for(auto& param : mParams) {
        param->findUsedVariables(searcher);
    }
}
FunctionChainExpressionNode::FunctionChainExpressionNode(SourceCodeRef source, up<ExpressionNode> initialValue, up<FunctionCallExpressionNode> functionCall)
: ExpressionNode(std::move(source)), mInitialValue(std::move(initialValue)), mFunctionCall(std::move(functionCall)) {
}
Datatype FunctionChainExpressionNode::compile(Compiler& comp) const {
    return comp.compileChainedFunctionCall(*this);
}
std::string FunctionChainExpressionNode::dump(unsigned int indent) const {
    auto ret = ASTNode::dump(indent);
    ret += createIndent(indent + 1) + "Initial value:\n";
    ret += mInitialValue->dump(indent + 2);
    ret += createIndent(indent + 1) + "Function:\n";
    ret += mFunctionCall->dump(indent + 2);
    return ret;
}
void FunctionChainExpressionNode::findUsedVariables(VariableSearcher& searcher) const {
    mInitialValue->findUsedVariables(searcher);
    mFunctionCall->findUsedVariables(searcher);
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
void ListAccessExpressionNode::findUsedVariables(VariableSearcher&) const {
    todo();
}

ListPropertyAccessExpression::ListPropertyAccessExpression(SourceCodeRef source, up<ExpressionNode> list, ListPropertyAccessExpression::ListProperty property)
: ExpressionNode(source), mList(std::move(list)), mProperty(std::move(property)) {
}
Datatype ListPropertyAccessExpression::compile(Compiler& comp) const {
    return comp.compileListPropertyAccess(*this);
}
void ListPropertyAccessExpression::findUsedVariables(VariableSearcher& searcher) const {
    mList->findUsedVariables(searcher);
}
std::string ListPropertyAccessExpression::dump(unsigned int indent) const {
    return ASTNode::dump(indent);
}

TupleAccessExpressionNode::TupleAccessExpressionNode(SourceCodeRef source, up<ExpressionNode> name, uint32_t index)
: ExpressionNode(std::move(source)), mName(std::move(name)), mIndex(index) {
}
Datatype TupleAccessExpressionNode::compile(Compiler& comp) const {
    return comp.compileTupleAccessExpression(*this);
}
std::string TupleAccessExpressionNode::dump(unsigned int indent) const {
    auto ret = ASTNode::dump(indent);
    ret += createIndent(indent + 1) + "Name:\n";
    ret += mName->dump(indent + 2);
    ret += createIndent(indent + 1) + "Index:";
    ret += std::to_string(mIndex) + "\n";
    return ret;
}
void TupleAccessExpressionNode::findUsedVariables(VariableSearcher& searcher) const {
    mName->findUsedVariables(searcher);
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
    assert(false);
    return Datatype{};
}
const std::vector<up<DeclarationNode>>& ModuleRootNode::getDeclarations() const {
    return mDeclarations;
}
const std::string& ModuleRootNode::getModuleName() const {
    return mName;
}
void ModuleRootNode::findUsedVariables(VariableSearcher& searcher) const {
    for(auto& decl : mDeclarations) {
        decl->findUsedVariables(searcher);
    }
}

CallableDeclarationNode::CallableDeclarationNode(SourceCodeRef source)
: DeclarationNode(source) {
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
: CallableDeclarationNode(std::move(source)), mName(std::move(name)), mParameters(std::move(params)), mReturnType(std::move(returnType)), mBody(std::move(body)) {
}
Datatype FunctionDeclarationNode::compile(Compiler& comp) const {
    comp.compileFunction(*this);
    return getDatatype();
}
bool FunctionDeclarationNode::hasTemplateParameters() const {
    return !mName->getTemplateParameters().empty();
}
std::vector<Datatype> FunctionDeclarationNode::getTemplateParameterVector() const {
    return mName->getTemplateParameters();
}
const IdentifierNode* FunctionDeclarationNode::getIdentifier() const {
    return mName.get();
}
Datatype FunctionDeclarationNode::getDatatype() const {
    return getFunctionType(mReturnType, mParameters);
}
void FunctionDeclarationNode::findUsedVariables(VariableSearcher& searcher) const {
    mName->findUsedVariables(searcher);
    for(auto& param : mParameters) {
        param.name->findUsedVariables(searcher);
    }
    mBody->findUsedVariables(searcher);
}

NativeFunctionDeclarationNode::NativeFunctionDeclarationNode(SourceCodeRef source, up<IdentifierNode> name, std::vector<Parameter> params, Datatype returnType)
: CallableDeclarationNode(std::move(source)), mName(std::move(name)), mParameters(std::move(params)), mReturnType(std::move(returnType)) {
}
bool NativeFunctionDeclarationNode::hasTemplateParameters() const {
    return !mName->getTemplateParameters().empty();
}
std::vector<Datatype> NativeFunctionDeclarationNode::getTemplateParameterVector() const {
    return mName->getTemplateParameters();
}
const IdentifierNode* NativeFunctionDeclarationNode::getIdentifier() const {
    return mName.get();
}
Datatype NativeFunctionDeclarationNode::getDatatype() const {
    return getFunctionType(mReturnType, mParameters);
}
Datatype NativeFunctionDeclarationNode::compile(Compiler& comp) const {
    assert(false);
}
void NativeFunctionDeclarationNode::findUsedVariables(VariableSearcher& searcher) const {
    mName->findUsedVariables(searcher);
    for(auto& param : mParameters) {
        param.name->findUsedVariables(searcher);
    }
}
std::string NativeFunctionDeclarationNode::dump(unsigned int indent) const {
    auto ret = ASTNode::dump(indent);
    ret += createIndent(indent + 1) + "Name:\n" + mName->dump(indent + 2);
    ret += createIndent(indent + 1) + "Returns: " + mReturnType.toString() + "\n";
    ret += createIndent(indent + 1) + "Params: \n" + dumpParameterVector(indent + 2, mParameters);
    return ret;
}

StructDeclarationNode::StructDeclarationNode(SourceCodeRef source, up<IdentifierNode> name, std::vector<Parameter> values)
: DeclarationNode(std::move(source)), mName(std::move(name)), mValues(std::move(values)) {
}
bool StructDeclarationNode::hasTemplateParameters() const {
    return !mName->getTemplateParameters().empty();
}
std::vector<Datatype> StructDeclarationNode::getTemplateParameterVector() const {
    return mName->getTemplateParameters();
}
const IdentifierNode* StructDeclarationNode::getIdentifier() const {
    return mName.get();
}
Datatype StructDeclarationNode::compile(Compiler& comp) const {
    return ASTNode::compile(comp);
}
void StructDeclarationNode::findUsedVariables(VariableSearcher&) const {
}
std::string StructDeclarationNode::dump(unsigned int indent) const {
    return ASTNode::dump(indent);
}

Datatype getFunctionType(const Datatype& returnType, const std::vector<Parameter>& params) {
    std::vector<Datatype> parameterTypes;
    parameterTypes.reserve(params.size());
    for(auto& param : params) {
        parameterTypes.push_back(param.type);
    }
    return Datatype::createFunctionType(returnType, std::move(parameterTypes));
}
}