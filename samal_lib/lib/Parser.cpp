#include "samal_lib/Parser.hpp"
#include "samal_lib/AST.hpp"
#include <charconv>
#include <iostream>

namespace samal {

static SourceCodeRef toRef(const peg::MatchInfo& res) {
    return SourceCodeRef{
        .start = res.startTrimmed(),
        .len = res.len,
        .line = res.sourcePosition.first,
        .column = res.sourcePosition.second
    };
}

Parser::Parser() {
    Stopwatch stopwatch{ "Parser construction" };
    mPegParser["Start"] << "Declaration+" >> [](peg::MatchInfo& res) -> peg::Any {
        std::vector<up<DeclarationNode>> decls;
        for(auto& d : res.subs) {
            decls.emplace_back(d.result.move<DeclarationNode*>());
        }
        return ModuleRootNode{ toRef(res), std::move(decls) };
    };
    mPegParser["Declaration"] << "FunctionDeclaration | NativeFunctionDeclaration" >> [](peg::MatchInfo& res) -> peg::Any {
        return std::move(res[0].result);
    };
    mPegParser["FunctionDeclaration"] << "'fn' IdentifierWithTemplate '(' ParameterVector ')' '->' Datatype ScopeExpression" >> [](peg::MatchInfo& res) -> peg::Any {
        return FunctionDeclarationNode(
            toRef(res),
            up<IdentifierNode>{ res[1].result.move<IdentifierNode*>() },
            std::vector<Parameter>{ res.subs.at(3).result.moveValue<std::vector<Parameter>>() },
            res.subs.at(6).result.moveValue<Datatype>(),
            up<ScopeNode>{ res.subs.at(7).result.move<ScopeNode*>() });
    };
    mPegParser["NativeFunctionDeclaration"] << "'native' 'fn' IdentifierWithTemplate '(' ParameterVector ')' '->' Datatype" >> [](peg::MatchInfo& res) -> peg::Any {
        return NativeFunctionDeclarationNode(
            toRef(res),
            up<IdentifierNode>{ res[2].result.move<IdentifierNode*>() },
            std::vector<Parameter>{ res[4].result.moveValue<std::vector<Parameter>>() },
            res[7].result.moveValue<Datatype>());
    };
    mPegParser["ParameterVector"] << "ParameterVectorRec?" >> [](peg::MatchInfo& res) -> peg::Any {
        if(res.subs.empty())
            return std::vector<Parameter>{};
        return std::move(res[0].result);
    };
    mPegParser["ParameterVectorRec"] << "Identifier ':' Datatype (',' ParameterVectorRec)?" >> [](peg::MatchInfo& res) {
        std::vector<Parameter> params;
        params.emplace_back(Parameter{ up<IdentifierNode>{ res[0].result.move<IdentifierNode*>() }, res[2].result.moveValue<Datatype>() });
        // recursive child
        if(!res[3].subs.empty()) {
            auto childParams = res[3][0][1].result.moveValue<std::vector<Parameter>>();
            params.insert(params.end(), std::move_iterator(childParams.begin()), std::move_iterator(childParams.end()));
        }
        return params;
    };
    mPegParser["Identifier"] << "~sws~(~nws~(~nws~[a-zA-Z]+ ~nws~(~nws~[\\da-zA-Z])* ~nws~'.'?)+)" >> [](peg::MatchInfo& res) -> peg::Any {
        std::vector<std::string> parts;
        for(auto& part : res.subs) {
            const char* end = part.endTrimmed();
            size_t whiteSpacesToCut = 0;
            if(*(end - 1) == '.') {
                end--;
                whiteSpacesToCut += 1;
            }
            parts.emplace_back(std::string_view{ part.startTrimmed(), part.len - whiteSpacesToCut });
        }
        if(parts.size() > 2) {
            throw std::runtime_error{ "An identifier can only contain at most a single dot" };
        }
        return IdentifierNode{ toRef(res), std::move(parts), {} };
    };
    mPegParser["IdentifierWithTemplate"] << "~sws~(~nws~(~nws~[a-zA-Z]+ ~nws~(~nws~[\\da-zA-Z])* ~nws~'.'?)+) ('<' DatatypeVector '>')?" >> [](peg::MatchInfo& res) -> peg::Any {
        std::vector<std::string> parts;
        for(auto& part : res[0].subs) {
            const char* end = part.endTrimmed();
            size_t whiteSpacesToCut = 0;
            if(*(end - 1) == '.') {
                end--;
                whiteSpacesToCut += 1;
            }
            parts.emplace_back(std::string_view{ part.startTrimmed(), part.len - whiteSpacesToCut });
        }
        if(parts.size() > 2) {
            throw std::runtime_error{ "An identifier can only contain at most a single dot" };
        }
        std::vector<Datatype> templateParameters;
        if(!res[1].subs.empty()) {
            templateParameters = res[1][0][1].result.moveValue<std::vector<Datatype>>();
        }
        return IdentifierNode{ toRef(res), std::move(parts), std::move(templateParameters) };
    };
    mPegParser["Datatype"] << "('fn' '(' DatatypeVector ')' '->' Datatype) | '[' Datatype ']' | 'i32' | 'i64' | 'bool' | Identifier | '(' Datatype ')' | '(' DatatypeVector ')'" >> [](peg::MatchInfo& res) -> peg::Any {
        switch(*res.choice) {
        case 0:
            return Datatype{ res[0][5].result.moveValue<Datatype>(), res[0][2].result.moveValue<std::vector<Datatype>>() };
        case 1:
            return Datatype::createListType(res[0][1].result.moveValue<Datatype>());
        case 2:
            return Datatype{ DatatypeCategory::i32 };
        case 3:
            return Datatype{ DatatypeCategory::i64 };
        case 4:
            return Datatype{ DatatypeCategory::bool_ };
        case 5:
            return Datatype{ std::string{ std::string_view(res.startTrimmed(), static_cast<size_t>(res.endTrimmed() - res.startTrimmed())) } };
        case 6:
            return res[0][1].result.moveValue<Datatype>();
        case 7:
            return Datatype{ res[0][1].result.moveValue<std::vector<Datatype>>() };
        default:
            assert(false);
        }
    };
    mPegParser["DatatypeVector"] << "DatatypeVectorRec?" >> [](peg::MatchInfo& res) -> peg::Any {
        if(res.subs.empty())
            return std::vector<Datatype>{};
        return res[0].result.moveValue<std::vector<Datatype>>();
    };
    mPegParser["DatatypeVectorRec"] << "Datatype (',' DatatypeVectorRec)?" >> [](peg::MatchInfo& res) {
        std::vector<Datatype> params;
        params.emplace_back(res[0].result.moveValue<Datatype>());
        // recursive child
        if(!res[1].subs.empty()) {
            auto childParams = res[1][0][1].result.moveValue<std::vector<Datatype>>();
            params.insert(params.end(), std::move_iterator(childParams.begin()), std::move_iterator(childParams.end()));
        }
        return params;
    };
    mPegParser["ScopeExpression"] << "'{' (Expression ~snn~'\n')* '}'" >> [](peg::MatchInfo& res) {
        std::vector<up<ExpressionNode>> expressions;
        for(auto& expr : res[1].subs) {
            expressions.emplace_back(expr[0].result.move<ExpressionNode*>());
        }
        return ScopeNode{ toRef(res), std::move(expressions) };
    };
    mPegParser["Expression"] << "BasicExpression ('|>' BasicExpression)*" >> [](peg::MatchInfo& res) -> peg::Any {
        // create chained function calls
        peg::Any ret = std::move(res[0].result);
        while(!res[1].subs.empty()) {
            auto chainedFunctionCall = res[1][0][1].result.move<ExpressionNode*>();
            auto chainedFunctionCallCasted = dynamic_cast<FunctionCallExpressionNode*>(chainedFunctionCall);
            if(chainedFunctionCallCasted == nullptr) {
                throw std::runtime_error{ "Chained expression needs to be a function call, not a " + std::string{ chainedFunctionCall->getClassName() } };
            }
            ret = FunctionChainExpressionNode{
                toRef(res),
                up<ExpressionNode>{ ret.move<ExpressionNode*>() },
                up<FunctionCallExpressionNode>{ chainedFunctionCallCasted }
            };
            res[1].subs.erase(res[1].subs.begin());
        }
        return ret;
    };
    mPegParser["BasicExpression"] << "IfExpression | ScopeExpression | MathExpression" >> [](peg::MatchInfo& res) -> peg::Any {
        return std::move(res[0].result);
    };
    mPegParser["MathExpression"] << "AssignmentExpression #Expected mathematical expression#" >> [](peg::MatchInfo& res) -> peg::Any {
        return std::move(res.result);
    };
    mPegParser["AssignmentExpression"] << "(Identifier '=' Expression) | LogicalCombinationExpression" >> [](peg::MatchInfo& res) -> peg::Any {
        if(*res.choice == 0) {
            return AssignmentExpression{
                toRef(res),
                up<IdentifierNode>{ res[0][0].result.move<IdentifierNode*>() },
                up<ExpressionNode>{ res[0][2].result.move<ExpressionNode*>() }
            };
        }
        return std::move(res[0].result);
    };
    mPegParser["LogicalCombinationExpression"] << "LogicalEqualExpression (('&&' | '||') LogicalCombinationExpression)?" >> [](peg::MatchInfo& res) -> peg::Any {
        if(res[1].subs.empty()) {
            return std::move(res[0].result);
        }
        return BinaryExpressionNode{
            toRef(res),
            up<ExpressionNode>{ res[0].result.move<ExpressionNode*>() },
            *res[1][0][0].choice == 0 ? BinaryExpressionNode::BinaryOperator::LOGICAL_AND : BinaryExpressionNode::BinaryOperator::LOGICAL_OR,
            up<ExpressionNode>{ res[1][0][1].result.move<ExpressionNode*>() }
        };
    };
    mPegParser["LogicalEqualExpression"] << "LogicalComparisonExpression (('==' | '!=') LogicalEqualExpression)?" >> [](peg::MatchInfo& res) -> peg::Any {
        if(res[1].subs.empty()) {
            return std::move(res[0].result);
        }
        return BinaryExpressionNode{
            toRef(res),
            up<ExpressionNode>{ res[0].result.move<ExpressionNode*>() },
            *res[1][0][0].choice == 0 ? BinaryExpressionNode::BinaryOperator::LOGICAL_EQUALS : BinaryExpressionNode::BinaryOperator::LOGICAL_NOT_EQUALS,
            up<ExpressionNode>{ res[1][0][1].result.move<ExpressionNode*>() }
        };
    };
    mPegParser["LogicalComparisonExpression"] << "LineExpression (('<' | '<=' | '>=' | '>') LogicalComparisonExpression)?" >> [](peg::MatchInfo& res) -> peg::Any {
        if(res[1].subs.empty()) {
            return std::move(res[0].result);
        }
        BinaryExpressionNode::BinaryOperator op;
        switch(*res[1][0][0].choice) {
        case 0:
            op = BinaryExpressionNode::BinaryOperator::COMPARISON_LESS_THAN;
            break;
        case 1:
            op = BinaryExpressionNode::BinaryOperator::COMPARISON_LESS_EQUAL_THAN;
            break;
        case 2:
            op = BinaryExpressionNode::BinaryOperator::COMPARISON_MORE_EQUAL_THAN;
            break;
        case 3:
            op = BinaryExpressionNode::BinaryOperator::COMPARISON_MORE_THAN;
            break;
        default:
            assert(false);
        }
        return BinaryExpressionNode{
            toRef(res),
            up<ExpressionNode>{ res[0].result.move<ExpressionNode*>() },
            op,
            up<ExpressionNode>{ res[1][0][1].result.move<ExpressionNode*>() }
        };
    };
    mPegParser["LineExpression"] << "DotExpression (('+' | '-') LineExpression)?" >> [](peg::MatchInfo& res) -> peg::Any {
        if(res[1].subs.empty()) {
            return std::move(res[0].result);
        }
        return BinaryExpressionNode{
            toRef(res),
            up<ExpressionNode>{ res[0].result.move<ExpressionNode*>() },
            *res[1][0][0].choice == 0 ? BinaryExpressionNode::BinaryOperator::PLUS : BinaryExpressionNode::BinaryOperator::MINUS,
            up<ExpressionNode>{ res[1][0][1].result.move<ExpressionNode*>() }
        };
    };

    mPegParser["DotExpression"] << "PostfixExpression (('*' | '/') DotExpression)?" >> [](peg::MatchInfo& res) -> peg::Any {
        if(res[1].subs.empty()) {
            return std::move(res[0].result);
        }
        return BinaryExpressionNode{
            toRef(res),
            up<ExpressionNode>{ res[0].result.move<ExpressionNode*>() },
            *res[1][0][0].choice == 0 ? BinaryExpressionNode::BinaryOperator::MULTIPLY : BinaryExpressionNode::BinaryOperator::DIVIDE,
            up<ExpressionNode>{ res[1][0][1].result.move<ExpressionNode*>() }
        };
    };
    // this is stupid because we don't handle left recursion correctly in the peg parser :c
    mPegParser["PostfixExpression"] << "LiteralExpression ~nws~(~nws~(~nws~'(' ExpressionVector ')') | ~nws~('[' Expression ']') | ~nws~(~nws~':' ~nws~[\\d]+))*" >> [](peg::MatchInfo& res) -> peg::Any {
        peg::Any ret = std::move(res[0].result);
        while(!res[1].subs.empty()) {
            switch(*res[1][0].choice) {
            case 0:
                ret = FunctionCallExpressionNode{
                    toRef(res),
                    up<ExpressionNode>{ ret.move<ExpressionNode*>() },
                    res[1][0][0][1].result.moveValue<std::vector<up<ExpressionNode>>>()
                };
                break;
            case 1:
                ret = ListAccessExpressionNode{
                    toRef(res),
                    up<ExpressionNode>{ ret.move<ExpressionNode*>() },
                    up<ExpressionNode>{ res[1][0][0][1].result.move<ExpressionNode*>() }
                };
                break;
            case 2: {
                uint32_t index;
                const auto& node = res[1][0][0][1];
                std::from_chars(node.start, node.start + node.len, index);
                ret = TupleAccessExpressionNode{
                    toRef(res),
                    up<ExpressionNode>{ ret.move<ExpressionNode*>() },
                    index
                };
                break;
            }
            default:
                assert(false);
            }
            res[1].subs.erase(res[1].subs.begin());
        }
        return ret;
    };
    // TODO maybe combine MathExpression (for stuff like (5 + 3)) and ExpressionVector
    //  (for tuples like (5 + 3, 2) to prevent double parsing of the first expression; this could also
    //  be prevented by using packrat parsing in the peg parser :^).
    mPegParser["LiteralExpression"] << "~sws~(~nws~[\\d]+ ~nws~'i64'?) | 'true' | 'false' | '[' ':' Datatype ']' |  '[' ExpressionVector ']' | LambdaCreationExpression | IdentifierWithTemplate | '(' Expression ')' | '(' ExpressionVector ')' |  ScopeExpression" >> [](peg::MatchInfo& res) -> peg::Any {
        switch(*res.choice) {
        case 0:
            if(res[0][1].subs.empty()) {
                int32_t val;
                std::from_chars(res.startTrimmed(), res.endTrimmed(), val);
                return LiteralInt32Node{ toRef(res), val };
            } else {
                int64_t val;
                std::from_chars(res.startTrimmed(), res.endTrimmed(), val);
                return LiteralInt64Node{ toRef(res), val };
            }
        case 1:
            return LiteralBoolNode{ toRef(res), true };
        case 2:
            return LiteralBoolNode{ toRef(res), false };
        case 3:
            return ListCreationNode(toRef(res), res[0][2].result.moveValue<Datatype>());
        case 4:
            return ListCreationNode(toRef(res), res[0][1].result.moveValue<std::vector<up<ExpressionNode>>>());
        case 5:
        case 6:
        case 9:
            return std::move(res[0].result);
        case 7:
            return std::move(res[0][1].result);
        case 8:
            return TupleCreationNode{ toRef(res), res[0][1].result.moveValue<std::vector<up<ExpressionNode>>>() };
        default:
            assert(false);
        }
    };
    mPegParser["LambdaCreationExpression"] << "'fn' '(' ParameterVector ')' '->' Datatype ScopeExpression" >> [](peg::MatchInfo& res) -> peg::Any {
        return LambdaCreationNode{
            toRef(res),
            std::vector<Parameter>{ res[2].result.moveValue<std::vector<Parameter>>() },
            res[5].result.moveValue<Datatype>(),
            up<ScopeNode>{ res[6].result.move<ScopeNode*>() }
        };
    };
    mPegParser["IfExpression"] << "'if' Expression ScopeExpression ('else' 'if' Expression ScopeExpression)* ('else' ScopeExpression)?" >> [](peg::MatchInfo& res) -> peg::Any {
        IfExpressionChildList list;
        list.push_back(std::make_pair(
            up<ExpressionNode>{ res[1].result.move<ExpressionNode*>() },
            up<ScopeNode>{ res[2].result.move<ScopeNode*>() }));
        for(auto& elseIf : res[3].subs) {
            list.push_back(std::make_pair(
                up<ExpressionNode>{ elseIf[2].result.move<ExpressionNode*>() },
                up<ScopeNode>{ elseIf[3].result.move<ScopeNode*>() }));
        }
        up<ScopeNode> elseBody;
        if(!res[4].subs.empty()) {
            elseBody.reset(res[4][0][1].result.move<ScopeNode*>());
        }
        return IfExpressionNode{ toRef(res), std::move(list), std::move(elseBody) };
    };
    mPegParser["ExpressionVector"] << "ExpressionVectorRec?" >> [](peg::MatchInfo& res) -> peg::Any {
        if(res.subs.empty())
            return std::vector<up<ExpressionNode>>{};
        return std::move(res[0].result);
    };
    mPegParser["ExpressionVectorRec"] << "Expression (',' ExpressionVectorRec)?" >> [](peg::MatchInfo& res) {
        std::vector<up<ExpressionNode>> params;
        params.emplace_back(up<ExpressionNode>{ res[0].result.move<ExpressionNode*>() });
        // recursive child
        if(!res[1].subs.empty()) {
            auto childParams = res[1][0][1].result.moveValue<std::vector<up<ExpressionNode>>>();
            params.insert(params.end(), std::move_iterator(childParams.begin()), std::move_iterator(childParams.end()));
        }
        return params;
    };
}

std::pair<up<ModuleRootNode>, peg::PegTokenizer> Parser::parse(std::string moduleName, std::string code) const {
    Stopwatch stopwatch{ "Parsing the code" };
    auto ret = mPegParser.parse("Start", std::move(code));
    if(ret.first.index() == 0) {
        up<ModuleRootNode> root{ std::get<0>(ret.first).moveMatchInfo().result.move<ModuleRootNode*>() };
        root->setModuleName(std::move(moduleName));
        return std::make_pair(std::move(root), std::move(ret.second));
    }

    std::cerr << peg::errorsToString(std::get<1>(ret.first), ret.second);
    return std::make_pair(up<ModuleRootNode>{}, std::move(ret.second));
}
}