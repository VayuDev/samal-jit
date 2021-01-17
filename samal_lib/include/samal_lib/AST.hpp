#pragma once
#include "Datatype.hpp"
#include "samal_lib/Forward.hpp"
#include <optional>
#include <string>

namespace samal {

struct SourceCodeRef {
    const char* start = nullptr;
    size_t len = 0;
    size_t line = 0, column = 0;
};

class ASTNode {
public:
    explicit ASTNode(SourceCodeRef source);
    virtual ~ASTNode() = default;
    virtual void completeDatatype(DatatypeCompleter&){};
    virtual void compile(Compiler&) const {};
    [[nodiscard]] virtual std::string dump(unsigned indent) const;
    [[nodiscard]] virtual inline const char* getClassName() const { return "ASTNode"; }

protected:
    void throwException(const std::string& msg) const;

private:
    SourceCodeRef mSourceCodeRef;
};

class ParameterListNode : public ASTNode {
public:
    struct Parameter {
        up<IdentifierNode> name;
        Datatype type;
    };
    explicit ParameterListNode(SourceCodeRef source, std::vector<Parameter> params);
    void completeDatatype(DatatypeCompleter& declList) override;
    [[nodiscard]] const std::vector<Parameter>& getParams() const;
    [[nodiscard]] std::string dump(unsigned indent) const override;
    [[nodiscard]] inline const char* getClassName() const override { return "ParameterListNode"; }

private:
    std::vector<Parameter> mParams;
};

class ExpressionListNodeWithoutDatatypes : public ASTNode {
public:
    explicit ExpressionListNodeWithoutDatatypes(SourceCodeRef source, std::vector<up<ExpressionNode>> params);
    void completeDatatype(DatatypeCompleter& declList) override;
    [[nodiscard]] const std::vector<up<ExpressionNode>>& getParams() const;
    [[nodiscard]] std::string dump(unsigned indent) const override;
    [[nodiscard]] inline const char* getClassName() const override { return "ParameterListNodeWithoutDatatypes"; }

private:
    std::vector<up<ExpressionNode>> mParams;
};

class ExpressionNode : public ASTNode {
public:
    explicit ExpressionNode(SourceCodeRef source);
    [[nodiscard]] virtual std::optional<Datatype> getDatatype() const = 0;
    [[nodiscard]] inline const char* getClassName() const override { return "ExpressionNode"; }

private:
};

class AssignmentExpression : public ExpressionNode {
public:
    AssignmentExpression(SourceCodeRef source, up<IdentifierNode> left, up<ExpressionNode> right);
    void completeDatatype(DatatypeCompleter& declList) override;
    void compile(Compiler&) const override;
    const up<IdentifierNode>& getLeft() const;
    const up<ExpressionNode>& getRight() const;
    [[nodiscard]] std::optional<Datatype> getDatatype() const override;
    [[nodiscard]] std::string dump(unsigned indent) const override;
    [[nodiscard]] inline const char* getClassName() const override { return "AssignmentExpression"; }

private:
    up<IdentifierNode> mLeft;
    up<ExpressionNode> mRight;
};

class BinaryExpressionNode : public ExpressionNode {
public:
    enum class BinaryOperator {
        PLUS,
        MINUS,
        MULTIPLY,
        DIVIDE,
        LOGICAL_AND,
        LOGICAL_OR,
        LOGICAL_EQUALS,
        LOGICAL_NOT_EQUALS,
        COMPARISON_LESS_THAN,
        COMPARISON_LESS_EQUAL_THAN,
        COMPARISON_MORE_THAN,
        COMPARISON_MORE_EQUAL_THAN,

    };
    BinaryExpressionNode(SourceCodeRef source, up<ExpressionNode> left, BinaryOperator op, up<ExpressionNode> right);
    void completeDatatype(DatatypeCompleter& declList) override;
    void compile(Compiler&) const override;
    [[nodiscard]] std::optional<Datatype> getDatatype() const override;
    [[nodiscard]] std::string dump(unsigned indent) const override;
    [[nodiscard]] inline const char* getClassName() const override { return "BinaryExpressionNode"; }

private:
    up<ExpressionNode> mLeft;
    BinaryOperator mOperator;
    up<ExpressionNode> mRight;
};

class LiteralNode : public ExpressionNode {
public:
    explicit LiteralNode(SourceCodeRef source);
    [[nodiscard]] inline const char* getClassName() const override { return "LiteralNode"; }

private:
};

class LiteralInt32Node : public LiteralNode {
public:
    explicit LiteralInt32Node(SourceCodeRef source, int32_t val);
    void completeDatatype(DatatypeCompleter& declList) override;
    void compile(Compiler&) const override;
    [[nodiscard]] std::optional<Datatype> getDatatype() const override;
    [[nodiscard]] std::string dump(unsigned indent) const override;
    [[nodiscard]] inline const char* getClassName() const override { return "LiteralInt32Node"; }

private:
    int32_t mValue;
};

class LiteralInt64Node : public LiteralNode {
public:
    explicit LiteralInt64Node(SourceCodeRef source, int64_t val);
    void completeDatatype(DatatypeCompleter& declList) override;
    void compile(Compiler&) const override;
    [[nodiscard]] std::optional<Datatype> getDatatype() const override;
    [[nodiscard]] std::string dump(unsigned indent) const override;
    [[nodiscard]] inline const char* getClassName() const override { return "LiteralInt64Node"; }

private:
    int64_t mValue;
};

class LiteralBoolNode : public LiteralNode {
public:
    explicit LiteralBoolNode(SourceCodeRef source, bool val);
    void completeDatatype(DatatypeCompleter& declList) override;
    void compile(Compiler&) const override;
    [[nodiscard]] std::optional<Datatype> getDatatype() const override;
    [[nodiscard]] std::string dump(unsigned indent) const override;
    [[nodiscard]] inline const char* getClassName() const override { return "LiteralBoolNode"; }

private:
    bool mValue;
};

class IdentifierNode : public ExpressionNode {
public:
    explicit IdentifierNode(SourceCodeRef source, std::vector<std::string> name, std::vector<Datatype> templateParameters);
    void completeDatatype(DatatypeCompleter& declList) override;
    void compile(Compiler&) const override;
    [[nodiscard]] std::string getName() const;
    [[nodiscard]] std::optional<Datatype> getDatatype() const override;
    [[nodiscard]] std::optional<int32_t> getId() const;
    [[nodiscard]] std::string dump(unsigned indent) const override;
    [[nodiscard]] inline const char* getClassName() const override { return "IdentifierNode"; }

private:
    std::vector<std::string> mName;
    std::optional<std::pair<Datatype, int32_t>> mDatatype;
    std::vector<Datatype> mTemplateParameters;
};

class TupleCreationNode : public ExpressionNode {
public:
    explicit TupleCreationNode(SourceCodeRef source, up<ExpressionListNodeWithoutDatatypes> params);
    void completeDatatype(DatatypeCompleter& declList) override;
    void compile(Compiler&) const override;
    [[nodiscard]] std::optional<Datatype> getDatatype() const override;
    [[nodiscard]] std::string dump(unsigned indent) const override;
    [[nodiscard]] inline const char* getClassName() const override { return "TupleCreationNode"; }

private:
    up<ExpressionListNodeWithoutDatatypes> mParams;
};

class ListCreationNode : public ExpressionNode {
public:
    explicit ListCreationNode(SourceCodeRef source, up<ExpressionListNodeWithoutDatatypes> params);
    explicit ListCreationNode(SourceCodeRef source, Datatype baseType);
    void completeDatatype(DatatypeCompleter& declList) override;
    //void compile(Compiler &) const override;
    [[nodiscard]] std::optional<Datatype> getDatatype() const override;
    [[nodiscard]] std::string dump(unsigned indent) const override;
    [[nodiscard]] inline const char* getClassName() const override { return "ListCreationNode"; }

private:
    std::optional<Datatype> mBaseType;
    up<ExpressionListNodeWithoutDatatypes> mParams;
};

class ScopeNode : public ExpressionNode {
public:
    explicit ScopeNode(SourceCodeRef source, std::vector<up<ExpressionNode>> expressions);
    void completeDatatype(DatatypeCompleter& declList) override;
    void compile(Compiler&) const override;
    [[nodiscard]] std::optional<Datatype> getDatatype() const override;
    [[nodiscard]] std::string dump(unsigned indent) const override;
    [[nodiscard]] inline const char* getClassName() const override { return "ScopeNode"; }

private:
    std::vector<up<ExpressionNode>> mExpressions;
};

using IfExpressionChildList = std::vector<std::pair<up<ExpressionNode>, up<ScopeNode>>>;
class IfExpressionNode : public ExpressionNode {
public:
    IfExpressionNode(SourceCodeRef source, IfExpressionChildList children, up<ScopeNode> elseBody);
    void completeDatatype(DatatypeCompleter& declList) override;
    void compile(Compiler&) const override;
    [[nodiscard]] std::optional<Datatype> getDatatype() const override;
    [[nodiscard]] std::string dump(unsigned indent) const override;
    [[nodiscard]] inline const char* getClassName() const override { return "IfExpressionNode"; }

private:
    IfExpressionChildList mChildren;
    up<ScopeNode> mElseBody;
};

class FunctionCallExpressionNode : public ExpressionNode {
public:
    FunctionCallExpressionNode(SourceCodeRef source, up<ExpressionNode> name, up<ExpressionListNodeWithoutDatatypes> params);
    [[nodiscard]] std::optional<Datatype> getDatatype() const override;
    void completeDatatype(DatatypeCompleter& declList) override;
    void compile(Compiler&) const override;
    [[nodiscard]] std::string dump(unsigned indent) const override;
    [[nodiscard]] inline const char* getClassName() const override { return "FunctionCallExpressionNode"; }
    void prependChainedParameter(Datatype chainedParamType);

private:
    up<ExpressionNode> mName;
    std::optional<Datatype> mPrependedChainedParameterType;
    up<ExpressionListNodeWithoutDatatypes> mParams;
};

class FunctionChainExpressionNode : public ExpressionNode {
public:
    FunctionChainExpressionNode(SourceCodeRef source, up<ExpressionNode> initialValue, up<FunctionCallExpressionNode> functionCall);
    [[nodiscard]] std::optional<Datatype> getDatatype() const override;
    void completeDatatype(DatatypeCompleter& declList) override;
    void compile(Compiler&) const override;
    [[nodiscard]] std::string dump(unsigned indent) const override;
    [[nodiscard]] inline const char* getClassName() const override { return "FunctionChainExpressionNode"; }

private:
    up<ExpressionNode> mInitialValue;
    up<FunctionCallExpressionNode> mFunctionCall;
};

class ListAccessExpressionNode : public ExpressionNode {
public:
    ListAccessExpressionNode(SourceCodeRef source, up<ExpressionNode> name, up<ExpressionNode> index);
    [[nodiscard]] std::optional<Datatype> getDatatype() const override;
    void completeDatatype(DatatypeCompleter& declList) override;
    //void compile(Compiler &) const override;
    [[nodiscard]] std::string dump(unsigned indent) const override;
    [[nodiscard]] inline const char* getClassName() const override { return "ListAccessExpressionNode"; }

private:
    up<ExpressionNode> mName, mIndex;
};

class TupleAccessExpressionNode : public ExpressionNode {
public:
    TupleAccessExpressionNode(SourceCodeRef source, up<ExpressionNode> name, uint32_t index);
    [[nodiscard]] std::optional<Datatype> getDatatype() const override;
    void completeDatatype(DatatypeCompleter& declList) override;
    void compile(Compiler&) const override;
    [[nodiscard]] std::string dump(unsigned indent) const override;
    [[nodiscard]] inline const char* getClassName() const override { return "TupleAccessExpressionNode"; }

private:
    up<ExpressionNode> mName;
    uint32_t mIndex;
};

class DeclarationNode : public ASTNode {
public:
    explicit DeclarationNode(SourceCodeRef source);
    virtual void declareShallow(DatatypeCompleter& completer) const = 0;
    //void compile(Compiler &) const override;
    [[nodiscard]] inline const char* getClassName() const override { return "DeclarationNode"; }

private:
};

class ModuleRootNode : public ASTNode {
public:
    explicit ModuleRootNode(SourceCodeRef source, std::vector<up<DeclarationNode>>&& declarations);
    void completeDatatype(DatatypeCompleter& declList) override;
    void declareShallow(DatatypeCompleter& completer) const;
    void compile(Compiler&) const override;
    [[nodiscard]] std::string dump(unsigned indent) const override;
    [[nodiscard]] inline const char* getClassName() const override { return "ModuleRootNode"; }
    std::vector<DeclarationNode*> createDeclarationList();
    void setModuleName(std::string name);

private:
    std::vector<up<DeclarationNode>> mDeclarations;
    std::string mName;
};

class FunctionDeclarationNode : public DeclarationNode {
public:
    FunctionDeclarationNode(SourceCodeRef source, up<IdentifierNode> name, up<ParameterListNode> params, Datatype returnType, up<ScopeNode> body);
    void completeDatatype(DatatypeCompleter& declList) override;
    void declareShallow(DatatypeCompleter& completer) const override;
    void compile(Compiler&) const override;
    [[nodiscard]] std::string dump(unsigned indent) const override;
    [[nodiscard]] inline const char* getClassName() const override { return "FunctionDeclarationNode"; }

private:
    up<IdentifierNode> mName;
    up<ParameterListNode> mParameters;
    Datatype mReturnType;
    up<ScopeNode> mBody;
};

}