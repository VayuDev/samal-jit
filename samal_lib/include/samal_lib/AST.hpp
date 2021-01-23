#pragma once
#include "Datatype.hpp"
#include "IdentifierId.hpp"
#include "samal_lib/Forward.hpp"
#include <optional>
#include <string>

namespace samal {

struct SourceCodeRef {
    const char* start = nullptr;
    size_t len = 0;
    size_t line = 0, column = 0;
};

struct Parameter {
    up<IdentifierNode> name;
    Datatype type;
};

class ASTNode {
public:
    explicit ASTNode(SourceCodeRef source);
    virtual ~ASTNode() = default;
    virtual Datatype compile(Compiler& comp) const {
        return Datatype{ DatatypeCategory::invalid };
    };
    [[nodiscard]] virtual std::string dump(unsigned indent) const;
    [[nodiscard]] virtual inline const char* getClassName() const { return "ASTNode"; }
    void throwException(const std::string& msg) const;

private:
    SourceCodeRef mSourceCodeRef;
};

class ExpressionNode : public ASTNode {
public:
    explicit ExpressionNode(SourceCodeRef source);
    [[nodiscard]] inline const char* getClassName() const override { return "ExpressionNode"; }

private:
};

class AssignmentExpression : public ExpressionNode {
public:
    AssignmentExpression(SourceCodeRef source, up<IdentifierNode> left, up<ExpressionNode> right);
    Datatype compile(Compiler& comp) const override;
    const up<IdentifierNode>& getLeft() const;
    const up<ExpressionNode>& getRight() const;
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
    Datatype compile(Compiler& comp) const override;
    [[nodiscard]] inline const auto& getLeft() const {
        return mLeft;
    }
    [[nodiscard]] inline auto getOperator() const {
        return mOperator;
    }
    [[nodiscard]] inline const auto& getRight() const {
        return mRight;
    }
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
    Datatype compile(Compiler& comp) const override;
    [[nodiscard]] std::string dump(unsigned indent) const override;
    [[nodiscard]] inline const char* getClassName() const override { return "LiteralInt32Node"; }

private:
    int32_t mValue;
};

class LiteralInt64Node : public LiteralNode {
public:
    explicit LiteralInt64Node(SourceCodeRef source, int64_t val);
    Datatype compile(Compiler& comp) const override;
    [[nodiscard]] std::string dump(unsigned indent) const override;
    [[nodiscard]] inline const char* getClassName() const override { return "LiteralInt64Node"; }

private:
    int64_t mValue;
};

class LiteralBoolNode : public LiteralNode {
public:
    explicit LiteralBoolNode(SourceCodeRef source, bool val);
    Datatype compile(Compiler& comp) const override;
    [[nodiscard]] std::string dump(unsigned indent) const override;
    [[nodiscard]] inline const char* getClassName() const override { return "LiteralBoolNode"; }

private:
    bool mValue;
};

class IdentifierNode : public ExpressionNode {
public:
    explicit IdentifierNode(SourceCodeRef source, std::vector<std::string> name, std::vector<Datatype> templateParameters);
    Datatype compile(Compiler& comp) const override;
    [[nodiscard]] std::string getName() const;
    [[nodiscard]] std::vector<std::string> getNameSplit() const;
    [[nodiscard]] const std::vector<Datatype>& getTemplateParameters() const;
    [[nodiscard]] std::string dump(unsigned indent) const override;
    [[nodiscard]] inline const char* getClassName() const override { return "IdentifierNode"; }

private:
    std::vector<std::string> mName;
    std::vector<Datatype> mTemplateParameters;
};

class TupleCreationNode : public ExpressionNode {
public:
    explicit TupleCreationNode(SourceCodeRef source, std::vector<up<ExpressionNode>> params);
    Datatype compile(Compiler& comp) const override;
    [[nodiscard]] std::string dump(unsigned indent) const override;
    [[nodiscard]] inline const char* getClassName() const override { return "TupleCreationNode"; }

private:
    std::vector<up<ExpressionNode>> mParams;
};

class ListCreationNode : public ExpressionNode {
public:
    explicit ListCreationNode(SourceCodeRef source, std::vector<up<ExpressionNode>> params);
    explicit ListCreationNode(SourceCodeRef source, Datatype baseType);
    [[nodiscard]] std::string dump(unsigned indent) const override;
    [[nodiscard]] inline const char* getClassName() const override { return "ListCreationNode"; }

private:
    std::optional<Datatype> mBaseType;
    std::vector<up<ExpressionNode>> mParams;
};

class ScopeNode : public ExpressionNode {
public:
    explicit ScopeNode(SourceCodeRef source, std::vector<up<ExpressionNode>> expressions);
    Datatype compile(Compiler& comp) const override;
    [[nodiscard]] inline const std::vector<up<ExpressionNode>>& getExpressions() const {
        return mExpressions;
    }
    [[nodiscard]] std::string dump(unsigned indent) const override;
    [[nodiscard]] inline const char* getClassName() const override { return "ScopeNode"; }

private:
    std::vector<up<ExpressionNode>> mExpressions;
};

using IfExpressionChildList = std::vector<std::pair<up<ExpressionNode>, up<ScopeNode>>>;
class IfExpressionNode : public ExpressionNode {
public:
    IfExpressionNode(SourceCodeRef source, IfExpressionChildList children, up<ScopeNode> elseBody);
    Datatype compile(Compiler& comp) const override;
    [[nodiscard]] inline const auto& getChildren() const {
        return mChildren;
    }
    [[nodiscard]] inline const auto& getElseBody() const {
        return mElseBody;
    }
    [[nodiscard]] std::string dump(unsigned indent) const override;
    [[nodiscard]] inline const char* getClassName() const override { return "IfExpressionNode"; }

private:
    IfExpressionChildList mChildren;
    up<ScopeNode> mElseBody;
};

class FunctionCallExpressionNode : public ExpressionNode {
public:
    FunctionCallExpressionNode(SourceCodeRef source, up<ExpressionNode> name, std::vector<up<ExpressionNode>> params);
    Datatype compile(Compiler& comp) const override;
    [[nodiscard]] std::string dump(unsigned indent) const override;
    [[nodiscard]] inline const char* getClassName() const override { return "FunctionCallExpressionNode"; }
    void prependChainedParameter(Datatype chainedParamType);

private:
    up<ExpressionNode> mName;
    std::optional<Datatype> mPrependedChainedParameterType;
    std::vector<up<ExpressionNode>> mParams;
};

class FunctionChainExpressionNode : public ExpressionNode {
public:
    FunctionChainExpressionNode(SourceCodeRef source, up<ExpressionNode> initialValue, up<FunctionCallExpressionNode> functionCall);

    Datatype compile(Compiler& comp) const override;
    [[nodiscard]] std::string dump(unsigned indent) const override;
    [[nodiscard]] inline const char* getClassName() const override { return "FunctionChainExpressionNode"; }

private:
    up<ExpressionNode> mInitialValue;
    up<FunctionCallExpressionNode> mFunctionCall;
};

class ListAccessExpressionNode : public ExpressionNode {
public:
    ListAccessExpressionNode(SourceCodeRef source, up<ExpressionNode> name, up<ExpressionNode> index);
    [[nodiscard]] std::string dump(unsigned indent) const override;
    [[nodiscard]] inline const char* getClassName() const override { return "ListAccessExpressionNode"; }

private:
    up<ExpressionNode> mName, mIndex;
};

class TupleAccessExpressionNode : public ExpressionNode {
public:
    TupleAccessExpressionNode(SourceCodeRef source, up<ExpressionNode> name, uint32_t index);

    Datatype compile(Compiler& comp) const override;
    [[nodiscard]] std::string dump(unsigned indent) const override;
    [[nodiscard]] inline const char* getClassName() const override { return "TupleAccessExpressionNode"; }

private:
    up<ExpressionNode> mName;
    uint32_t mIndex;
};

class DeclarationNode : public ASTNode {
public:
    explicit DeclarationNode(SourceCodeRef source);
    [[nodiscard]] virtual bool hasTemplateParameters() const = 0;
    [[nodiscard]] virtual std::vector<Datatype> getTemplateParameterVector() const = 0;
    [[nodiscard]] virtual const IdentifierNode* getIdentifier() const = 0;
    [[nodiscard]] virtual Datatype getDatatype() const = 0;
    //void compile(Compiler &) const override;
    [[nodiscard]] inline const char* getClassName() const override { return "DeclarationNode"; }

private:
};

class ModuleRootNode : public ASTNode {
public:
    explicit ModuleRootNode(SourceCodeRef source, std::vector<up<DeclarationNode>>&& declarations);
    Datatype compile(Compiler& comp) const override;
    [[nodiscard]] const std::vector<up<DeclarationNode>>& getDeclarations() const;
    [[nodiscard]] std::string dump(unsigned indent) const override;
    [[nodiscard]] inline const char* getClassName() const override { return "ModuleRootNode"; }
    std::vector<DeclarationNode*> createDeclarationList();
    void setModuleName(std::string name);
    [[nodiscard]] const std::string& getModuleName() const;

private:
    std::vector<up<DeclarationNode>> mDeclarations;
    std::string mName;
};

class FunctionDeclarationNode : public DeclarationNode {
public:
    FunctionDeclarationNode(SourceCodeRef source, up<IdentifierNode> name, std::vector<Parameter> params, Datatype returnType, up<ScopeNode> body);
    [[nodiscard]] bool hasTemplateParameters() const override;
    [[nodiscard]] std::vector<Datatype> getTemplateParameterVector() const override;
    [[nodiscard]] const IdentifierNode* getIdentifier() const override;
    [[nodiscard]] inline const std::vector<Parameter>& getParameters() const {
        return mParameters;
    }
    [[nodiscard]] inline const Datatype& getReturnType() const {
        return mReturnType;
    }
    [[nodiscard]] inline const up<ScopeNode>& getBody() const {
        return mBody;
    }
    [[nodiscard]] Datatype getDatatype() const override;
    Datatype compile(Compiler& comp) const override;
    [[nodiscard]] std::string dump(unsigned indent) const override;
    [[nodiscard]] inline const char* getClassName() const override { return "FunctionDeclarationNode"; }

private:
    up<IdentifierNode> mName;
    std::vector<Parameter> mParameters;
    Datatype mReturnType;
    up<ScopeNode> mBody;
};

}