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
    mPegParser["Declaration"] << "FunctionDeclaration | NativeFunctionDeclaration | StructDeclaration | EnumDeclaration" >> [](peg::MatchInfo& res) -> peg::Any {
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
    mPegParser["StructDeclaration"] << "'struct' IdentifierWithTemplate '{' ParameterVector '}'" >> [](peg::MatchInfo& res) -> peg::Any {
        return StructDeclarationNode(
            toRef(res),
            up<IdentifierNode>{ res[1].result.move<IdentifierNode*>() },
            res[3].result.moveValue<std::vector<Parameter>>());
    };
    mPegParser["EnumField"] << "Identifier '{' DatatypeVector '}'" >> [](peg::MatchInfo& res) -> peg::Any {
        up<IdentifierNode> identifier{res[0].result.move<IdentifierNode*>()};
        return EnumField{
            identifier->getName(),
            res[2].result.moveValue<std::vector<Datatype>>()};
    };
    mPegParser["EnumFieldVector"] << "EnumFieldVectorRec?" >> [](peg::MatchInfo& res) -> peg::Any {
        if(res.subs.empty())
            return std::vector<EnumField>{};
        return res[0].result.moveValue<std::vector<EnumField>>();
    };
    mPegParser["EnumFieldVectorRec"] << "EnumField (',' EnumFieldVectorRec)?" >> [](peg::MatchInfo& res) {
        std::vector<EnumField> params;
        params.emplace_back(res[0].result.moveValue<EnumField>());
        // recursive child
        if(!res[1].subs.empty()) {
            auto childParams = res[1][0][1].result.moveValue<std::vector<EnumField>>();
            params.insert(params.end(), std::move_iterator(childParams.begin()), std::move_iterator(childParams.end()));
        }
        return params;
    };
    mPegParser["EnumDeclaration"] << "'enum' IdentifierWithTemplate '{' EnumFieldVector '}'" >> [](peg::MatchInfo& res) -> peg::Any {
        return EnumDeclarationNode(
            toRef(res),
            up<IdentifierNode>{ res[1].result.move<IdentifierNode*>() },
            res[3].result.moveValue<std::vector<EnumField>>());
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
    mPegParser["IdentifierPartString"] << "~sws~(~nws~(~nws~[a-zA-Z] ~nws~(~nws~[\\da-zA-Z])*))" >> [](peg::MatchInfo& res) -> peg::Any {
        return std::string{std::string_view{res.startTrimmed(), static_cast<size_t>(res.endTrimmed() - res.startTrimmed())}};
    };
    mPegParser["Identifier"] << "IdentifierPartString ('.' IdentifierPartString)?" >> [](peg::MatchInfo& res) -> peg::Any {
        std::vector<std::string> parts;
        parts.emplace_back(res[0].result.moveValue<std::string>());
        if(!res[1].subs.empty()) {
           parts.emplace_back(res[1][0][1].result.moveValue<std::string>());
        }
        return IdentifierNode{ toRef(res), std::move(parts), {} };
    };
    mPegParser["IdentifierWithTemplate"] << "Identifier ('<' DatatypeVector '>')?" >> [](peg::MatchInfo& res) -> peg::Any {
        up<IdentifierNode> baseIdentifier{res[0].result.move<IdentifierNode*>()};
        std::vector<Datatype> templateParameters;
        if(!res[1].subs.empty()) {
            templateParameters = res[1][0][1].result.moveValue<std::vector<Datatype>>();
        }
        return IdentifierNode{ toRef(res), baseIdentifier->getNameSplit(),  std::move(templateParameters)};
    };
    mPegParser["Datatype"] << "('fn' '(' DatatypeVector ')' '->' Datatype) | '[' Datatype ']' | 'i32' | 'i64' | 'bool' | 'byte' | 'char' | IdentifierWithTemplate | '(' Datatype ')' | '(' DatatypeVector ')' | ('$' Datatype)" >> [](peg::MatchInfo& res) -> peg::Any {
        switch(*res.choice) {
        case 0:
            return Datatype::createFunctionType(res[0][5].result.moveValue<Datatype>(), res[0][2].result.moveValue<std::vector<Datatype>>());
        case 1:
            return Datatype::createListType(res[0][1].result.moveValue<Datatype>());
        case 2:
            return Datatype::createSimple(DatatypeCategory::i32);
        case 3:
            return Datatype::createSimple(DatatypeCategory::i64);
        case 4:
            return Datatype::createSimple(DatatypeCategory::bool_);
        case 5:
            return Datatype::createSimple(DatatypeCategory::byte);
        case 6:
            return Datatype::createSimple(DatatypeCategory::char_);
        case 7: {
            up<IdentifierNode> identifier{ res[0].result.move<IdentifierNode*>() };
            return Datatype::createUndeterminedIdentifierType(*identifier);
        }
        case 8:
            return res[0][1].result.moveValue<Datatype>();
        case 9:
            return Datatype::createTupleType(res[0][1].result.moveValue<std::vector<Datatype>>());
        case 10:
            return Datatype::createPointerType(res[0][1].result.moveValue<Datatype>());
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
    mPegParser["ScopeExpression"] << "'{' (Statement ~snn~'\n')* '}'" >> [](peg::MatchInfo& res) {
        std::vector<up<StatementNode>> expressions;
        for(auto& expr : res[1].subs) {
            expressions.emplace_back(expr[0].result.move<StatementNode*>());
        }
        return ScopeNode{ toRef(res), std::move(expressions) };
    };
    mPegParser["Statement"] << "('@tail_call_self(' ExpressionVector ')') | Expression" >> [](peg::MatchInfo& res) -> peg::Any {
        if(*res.choice == 1)
            return std::move(res[0].result);
        return TailCallSelfStatementNode{ toRef(res), res[0][1].result.moveValue<std::vector<up<ExpressionNode>>>() };
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

    mPegParser["DotExpression"] << "PostfixExpression (('*' | '/' | '%') DotExpression)?" >> [](peg::MatchInfo& res) -> peg::Any {
        if(res[1].subs.empty()) {
            return std::move(res[0].result);
        }
        BinaryExpressionNode::BinaryOperator op;
        switch(*res[1][0][0].choice) {
        case 0:
            op = BinaryExpressionNode::BinaryOperator::MULTIPLY;
            break;
        case 1:
            op = BinaryExpressionNode::BinaryOperator::DIVIDE;
            break;
        case 2:
            op = BinaryExpressionNode::BinaryOperator::MODULO;
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
    // this is stupid because we don't handle left recursion correctly in the peg parser :c
    mPegParser["PostfixExpression"] << "PrefixExpression ~nws~(~nws~(~nws~'(' ExpressionVector ')') | ~nws~(~nws~':' ~nws~[\\d]+) | ~nws~(~nws~':head') | ~nws~(~nws~':tail') | ~nws~(~nws~':' ~nws~IdentifierPartString))*" >> [](peg::MatchInfo& res) -> peg::Any {
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
            case 1: {
                uint32_t index;
                const auto& node = res[1][0][0][1];
                if(std::from_chars(node.start, node.start + node.len, index).ec != std::errc{}) {
                    throw std::runtime_error{"Unable to parse u32"};
                }
                ret = TupleAccessExpressionNode{
                    toRef(res),
                    up<ExpressionNode>{ ret.move<ExpressionNode*>() },
                    index
                };
                break;
            }
            case 2: {
                ret = ListPropertyAccessExpression{ toRef(res), up<ExpressionNode>{ ret.move<ExpressionNode*>() }, ListPropertyAccessExpression::ListProperty::HEAD };
                break;
            }
            case 3: {
                ret = ListPropertyAccessExpression{ toRef(res), up<ExpressionNode>{ ret.move<ExpressionNode*>() }, ListPropertyAccessExpression::ListProperty::TAIL };
                break;
            }
            case 4: {
                ret = StructFieldAccessExpression{ toRef(res), up<ExpressionNode>{ ret.move<ExpressionNode*>() }, res[1][0][0][1].result.moveValue<std::string>() };
                break;
            }
            default:
                assert(false);
            }
            res[1].subs.erase(res[1].subs.begin());
        }
        return ret;
    };
    mPegParser["PrefixExpression"] << "('!' PrefixExpression) | ('$' PrefixExpression) | ('@' PrefixExpression) | LiteralExpression" >> [](peg::MatchInfo& res) -> peg::Any {
        switch(*res.choice) {
        case 0:
            todo();
        case 1:
            return MoveToHeapExpression{toRef(res), up<ExpressionNode>{res[0][1].result.move<ExpressionNode*>()}};
        case 2:
            return MoveToStackExpression{toRef(res), up<ExpressionNode>{res[0][1].result.move<ExpressionNode*>()}};
        case 3:
            return std::move(res[0].result);
        }
        assert(false);
    };
    // TODO maybe combine MathExpression (for stuff like (5 + 3)) and ExpressionVector
    //  (for tuples like (5 + 3, 2) to prevent double parsing of the first expression; this could also
    //  be prevented by using packrat parsing in the peg parser :^).
    mPegParser["LiteralExpression"] << "~sws~(~nws~'-'? ~nws~[\\d]+ ~nws~(~nws~'b' | ~nws~'i64')?) | 'true' | 'false' | ('\\'' ~nws~LiteralCharRaw ~nws~'\\'') | LiteralString | '[' ':' Datatype ']' "
                                       " | '[' ExpressionVector ']' | LambdaCreationExpression "
                                       " | StructCreationExpression | EnumCreationExpression | MatchExpression | IdentifierWithTemplate | '(' Expression ')' | '(' ExpressionVector ')' |  ScopeExpression"
        >> [](peg::MatchInfo& res) -> peg::Any {
        switch(*res.choice) {
        case 0:
            if(res[0][2].subs.empty()) {
                int32_t val;
                if(std::from_chars(res.startTrimmed(), res.endTrimmed(), val).ec != std::errc{}) {
                    throw std::runtime_error{"Unable to parse i32"};
                }
                return LiteralInt32Node{ toRef(res), val };
            } else {
                switch(*res[0][2][0].choice) {
                case 0: {
                    uint8_t val;
                    if(std::from_chars(res.startTrimmed(), res.endTrimmed(), val).ec != std::errc{}) {
                        throw std::runtime_error{"Unable to parse byte"};
                    }
                    return LiteralByteNode{ toRef(res), val };
                }
                case 1: {
                    int64_t val;
                    if(std::from_chars(res.startTrimmed(), res.endTrimmed(), val).ec != std::errc{}) {
                        throw std::runtime_error{"Unable to parse i64"};
                    }
                    return LiteralInt64Node{ toRef(res), val };
                }
                }
                assert(false);
            }
        case 1:
            return LiteralBoolNode{ toRef(res), true };
        case 2:
            return LiteralBoolNode{ toRef(res), false };
        case 3:
            return LiteralCharNode{ toRef(res), res[0][1].result.moveValue<int32_t>() };
        case 5:
            return ListCreationNode(toRef(res), res[0][2].result.moveValue<Datatype>());
        case 6:
            return ListCreationNode(toRef(res), res[0][1].result.moveValue<std::vector<up<ExpressionNode>>>());
        case 4:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
        case 14:
            return std::move(res[0].result);
        case 12:
            return std::move(res[0][1].result);
        case 13:
            return TupleCreationNode{ toRef(res), res[0][1].result.moveValue<std::vector<up<ExpressionNode>>>() };
        default:
            assert(false);
        }
    };
    mPegParser["LiteralCharRaw"] << R"(~nws~'\"' | ~nws~'\n' | ~nws~'\r' | ~nws~'\0' | ~nws~(!~nws~'"' !~nws~'\'' ~nws~BUILTIN_ANY_UTF8_CODEPOINT))" >> [](peg::MatchInfo& res) -> peg::Any {
        int32_t charValue = 0;
        switch(*res.choice) {
        case 0:
            charValue = '"';
            break;
        case 1:
            charValue = '\n';
            break;
        case 2:
            charValue = '\r';
            break;
        case 3:
            charValue = '\0';
            break;
        case 4:
            charValue = res[0][2].result.moveValue<int32_t>();
            break;
        default:
            assert(false);
        }
        return peg::Any::create<int32_t>(std::move(charValue));
    };
    mPegParser["LiteralString"] << R"('"' ~nws~LiteralCharRaw* '"')" >> [](peg::MatchInfo& res) -> peg::Any {
        std::vector<up<ExpressionNode>> characterLiteralNodes;
        for(auto& ch: res[1].subs) {
            characterLiteralNodes.emplace_back(std::make_unique<LiteralCharNode>(toRef(res), ch.result.moveValue<int32_t>()));
        }
        if(characterLiteralNodes.empty()) {
            return ListCreationNode{toRef(res), Datatype::createSimple(DatatypeCategory::char_)};
        } else {
            return ListCreationNode{toRef(res), std::move(characterLiteralNodes)};
        }
    };
    mPegParser["MatchExpression"] << "'match' Expression '{' MatchCaseVector '}'" >> [](peg::MatchInfo& res) -> peg::Any {
        return MatchExpression{toRef(res),  up<ExpressionNode>{res[1].result.move<ExpressionNode*>()}, res[3].result.moveValue<std::vector<MatchCase>>()};
    };
    mPegParser["MatchCaseVector"] << "MatchCaseVectorRec?" >> [](peg::MatchInfo& res) -> peg::Any {
        if(res.subs.empty())
            return std::vector<MatchCase>{};
        return std::move(res[0].result);
    };
    mPegParser["MatchCaseVectorRec"] << "MatchCase (',' MatchCaseVectorRec)?" >> [](peg::MatchInfo& res) -> peg::Any {
        std::vector<MatchCase> params;
        up<MatchCase> thisCase{ res[0].result.move<MatchCase*>() };
        params.emplace_back(std::move(*thisCase));
        if(!res[1].subs.empty()) {
            auto childParams = res[1][0][1].result.moveValue<std::vector<MatchCase>>();
            params.insert(params.end(), std::move_iterator(childParams.begin()), std::move_iterator(childParams.end()));
        }
        return params;
    };
    mPegParser["MatchCase"] << "MatchCondition '->' Expression" >> [](peg::MatchInfo& res) -> peg::Any {
        return MatchCase{up<MatchCondition>{res[0].result.move<MatchCondition*>()}, up<ExpressionNode>{res[2].result.move<ExpressionNode*>()}};
    };
    mPegParser["MatchCondition"] << "('(' MatchConditionVector ')') | (IdentifierPartString ('{}' | '{' MatchConditionVector '}')) | IdentifierPartString" >> [](peg::MatchInfo& res) -> peg::Any {
        switch(*res.choice) {
        case 0:
            todo();
            break;
        case 1: {
            std::vector<up<MatchCondition>> matchConditions;
            if(*res[0][1].choice == 1) {
                matchConditions = res[0][1][0][1].result.moveValue<std::vector<up<MatchCondition>>>();
            }
            return EnumFieldMatchCondition{
                toRef(res),
                res[0][0].result.moveValue<std::string>(),
                std::move(matchConditions)
            };
        }
        case 2:
            return IdentifierMatchCondition{toRef(res), res[0].result.moveValue<std::string>()};
        }
        assert(false);
    };
    mPegParser["MatchConditionVector"] << "MatchConditionVectorRec?" >> [](peg::MatchInfo& res) -> peg::Any {
        if(res.subs.empty())
            return std::vector<up<MatchCondition>>{};
        return std::move(res[0].result);

    };
    mPegParser["MatchConditionVectorRec"] << "MatchCondition (',' MatchConditionVectorRec)?" >> [](peg::MatchInfo& res) -> peg::Any {
        std::vector<up<MatchCondition>> params;
        params.emplace_back(up<MatchCondition>{ res[0].result.move<MatchCondition*>() });
        if(!res[1].subs.empty()) {
            auto childParams = res[1][0][1].result.moveValue<std::vector<up<MatchCondition>>>();
            params.insert(params.end(), std::move_iterator(childParams.begin()), std::move_iterator(childParams.end()));
        }
        return params;
    };
    mPegParser["EnumCreationExpression"] << "Datatype '::' IdentifierPartString '{' ExpressionVector '}'" >> [](peg::MatchInfo& res) -> peg::Any {
        return EnumCreationNode{
            toRef(res),
            res[0].result.moveValue<Datatype>(),
            res[2].result.moveValue<std::string>(),
            res[4].result.moveValue<std::vector<up<ExpressionNode>>>()
        };
    };
    mPegParser["StructCreationExpression"] << "Datatype '{' StructParameterVector '}'" >> [](peg::MatchInfo& res) -> peg::Any {
        return StructCreationNode{
            toRef(res),
            res[0].result.moveValue<Datatype>(),
            res[2].result.moveValue<std::vector<StructCreationNode::StructCreationParameter>>()
        };
    };
    mPegParser["StructParameterVector"] << "StructParameterVectorRec?" >> [](peg::MatchInfo& res) -> peg::Any {
        if(res.subs.empty())
            return std::vector<StructCreationNode::StructCreationParameter>{};
        return std::move(res[0].result);
    };
    mPegParser["StructParameterVectorRec"] << "IdentifierPartString ':' Expression (',' StructParameterVectorRec)?" >> [](peg::MatchInfo& res) {
        auto identifierAsString = res[0].result.moveValue<std::string>();
        std::vector<StructCreationNode::StructCreationParameter> structParams;
        structParams.emplace_back(
            identifierAsString,
            up<ExpressionNode>{ res[2].result.move<ExpressionNode*>() });
        // recursive child
        if(!res[3].subs.empty()) {
            auto childParams = res[3][0][1].result.moveValue<std::vector<StructCreationNode::StructCreationParameter>>();
            structParams.insert(structParams.end(), std::move_iterator(childParams.begin()), std::move_iterator(childParams.end()));
        }
        return structParams;
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

std::pair<Datatype, peg::PegTokenizer> Parser::parseDatatype(std::string code) const {
    Stopwatch stopwatch{ "Parsing the code" };
    auto ret = mPegParser.parse("Datatype", code);
    if(ret.first.index() == 0) {
        return std::make_pair(std::get<0>(ret.first).getMatchInfoMut().result.moveValue<Datatype>(), ret.second);
    }
    throw std::runtime_error{"Unable to parse datatype from '" + code + "'"};
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