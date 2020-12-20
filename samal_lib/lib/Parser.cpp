#include "samal_lib/Parser.hpp"
#include "samal_lib/AST.hpp"
#include <iostream>
#include <charconv>

namespace samal {

Parser::Parser() {
  Stopwatch stopwatch{"Parser construction"};
  mPegParser["Start"] << "Declaration+" >> [] (peg::MatchInfo& res) -> peg::Any {
    std::vector<up<DeclarationNode>> decls;
    for(auto& d: res.subs) {
      decls.emplace_back(dynamic_cast<FunctionDeclarationNode*>(d.result.move<ASTNode*>()));
    }
    return ModuleRootNode{std::move(decls)};
  };
  mPegParser["Declaration"] << "FunctionDeclaration" >> [] (peg::MatchInfo& res) -> peg::Any {
    return std::move(res.result);
  };
  mPegParser["FunctionDeclaration"] << "'fn' Identifier '(' ParameterList ')' '->' Datatype ScopeExpression" >> [] (peg::MatchInfo& res) -> peg::Any {
    return FunctionDeclarationNode(
        up<IdentifierNode>{res[1].result.move<IdentifierNode*>()},
        up<ParameterListNode>{res.subs.at(3).result.move<ParameterListNode*>()},
        res.subs.at(6).result.moveValue<Datatype>(),
        up<ScopeNode>{res.subs.at(7).result.move<ScopeNode*>()});
  };
  mPegParser["ParameterList"] << "ParameterListRec?" >> [] (peg::MatchInfo& res) -> peg::Any {
    if(res.subs.empty())
      return ParameterListNode{{}};
    return ParameterListNode{res[0].result.moveValue<std::vector<ParameterListNode::Parameter>>()};
  };
  mPegParser["ParameterListRec"] << "Identifier ':' Datatype (',' ParameterListRec)?" >> [] (peg::MatchInfo& res) {
    std::vector<ParameterListNode::Parameter> params;
    params.emplace_back(ParameterListNode::Parameter{up<IdentifierNode>{res[0].result.move<IdentifierNode*>()}, res[2].result.moveValue<Datatype>()});
    // recursive child
    if(!res[3].subs.empty()) {
      auto childParams = res[3][0][1].result.moveValue<std::vector<ParameterListNode::Parameter>>();
      params.insert(params.end(), std::move_iterator(childParams.begin()), std::move_iterator(childParams.end()));
    }
    return params;
  };
  mPegParser["Identifier"] << "[a-zA-Z]+ ~nws~(~nws~[\\da-zA-Z])*" >> [] (peg::MatchInfo& res) -> peg::Any {
    return IdentifierNode{std::string{std::string_view{res.startTrimmed(), res.endTrimmed()}}};
  };
  mPegParser["Datatype"] << "'i32' | Identifier" >> [] (peg::MatchInfo& res) -> peg::Any {
    switch(*res.choice) {
      case 0:
        return Datatype{DatatypeCategory::i32};
      case 1:
        return Datatype{DatatypeCategory::undetermined_identifier};
      default:
        assert(false);
    }
  };
  mPegParser["ScopeExpression"] << "'{' (Expression ';')* '}'" >> [] (peg::MatchInfo& res) {
    std::vector<up<ExpressionNode>> expressions;
    for(auto& expr: res[1].subs) {
      expressions.emplace_back(expr[0].result.move<ExpressionNode*>());
    }
    return ScopeNode{std::move(expressions)};
  };
  mPegParser["Expression"] << "(IfExpression | ScopeExpression | MathExpression) #Expected Expression#" >> [] (peg::MatchInfo& res) -> peg::Any {
    return std::move(res[0].result);
  };
  mPegParser["MathExpression"] << "AssignmentExpression #Expected mathematical expression#" >> [] (peg::MatchInfo& res) -> peg::Any {
    return std::move(res.result);
  };
  mPegParser["AssignmentExpression"] << "(Identifier '=')? LogicalCombinationExpression" >> [] (peg::MatchInfo& res) -> peg::Any {
    if(!res[0].subs.empty()) {
      return AssignmentExpression{
        up<IdentifierNode>{res[0][0][0].result.move<IdentifierNode*>()},
        up<ExpressionNode>{res[1].result.move<ExpressionNode*>()}};
    }
    return std::move(res[1].result);
  };
  mPegParser["LogicalCombinationExpression"] << "LogicalEqualExpression (('&&' | '||') LogicalCombinationExpression)?" >> [] (peg::MatchInfo& res) -> peg::Any {
    if(res[1].subs.empty()) {
      return std::move(res[0].result);
    }
    return BinaryExpressionNode{
        up<ExpressionNode>{res[0].result.move<ExpressionNode*>()},
        *res[1][0][0].choice == 0 ? BinaryExpressionNode::BinaryOperator::LOGICAL_AND : BinaryExpressionNode::BinaryOperator::LOGICAL_OR,
        up<ExpressionNode>{res[1][0][1].result.move<ExpressionNode*>()}};
  };
  mPegParser["LogicalEqualExpression"] << "LogicalComparisonExpression (('==' | '!=') LogicalEqualExpression)?" >> [] (peg::MatchInfo& res) -> peg::Any {
    if(res[1].subs.empty()) {
      return std::move(res[0].result);
    }
    return BinaryExpressionNode{
        up<ExpressionNode>{res[0].result.move<ExpressionNode*>()},
        *res[1][0][0].choice == 0 ? BinaryExpressionNode::BinaryOperator::LOGICAL_EQUALS : BinaryExpressionNode::BinaryOperator::LOGICAL_NOT_EQUALS,
        up<ExpressionNode>{res[1][0][1].result.move<ExpressionNode*>()}};
  };
  mPegParser["LogicalComparisonExpression"] << "LineExpression (('<' | '<=' | '>=' | '>') LogicalComparisonExpression)?" >> [] (peg::MatchInfo& res) -> peg::Any {
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
        up<ExpressionNode>{res[0].result.move<ExpressionNode*>()},
        op,
        up<ExpressionNode>{res[1][0][1].result.move<ExpressionNode*>()}};
  };
  mPegParser["LineExpression"] << "DotExpression (('+' | '-') LineExpression)?" >> [] (peg::MatchInfo& res) -> peg::Any {
    if(res[1].subs.empty()) {
      return std::move(res[0].result);
    }
    return BinaryExpressionNode{
        up<ExpressionNode>{res[0].result.move<ExpressionNode*>()},
        *res[1][0][0].choice == 0 ? BinaryExpressionNode::BinaryOperator::PLUS : BinaryExpressionNode::BinaryOperator::MINUS,
        up<ExpressionNode>{res[1][0][1].result.move<ExpressionNode*>()}};
  };

  mPegParser["DotExpression"] << "LiteralExpression (('*' | '/') DotExpression)?" >> [] (peg::MatchInfo& res) -> peg::Any {
    if (res[1].subs.empty()) {
      return std::move(res[0].result);
    }
    return BinaryExpressionNode{
        up<ExpressionNode>{res[0].result.move<ExpressionNode *>()},
        *res[1][0][0].choice == 0 ? BinaryExpressionNode::BinaryOperator::MULTIPLY : BinaryExpressionNode::BinaryOperator::DIVIDE,
        up<ExpressionNode>{res[1][0][1].result.move<ExpressionNode *>()}};
  };
  mPegParser["LiteralExpression"] << "[\\d]+ | FunctionCallExpression | Identifier | '(' MathExpression ')' | ScopeExpression" >> [] (peg::MatchInfo& res) -> peg::Any {
    int32_t val;
    if(*res.choice == 0) {
      std::from_chars(res.startTrimmed(), res.endTrimmed(), val);
      return LiteralInt32Node{val};
    } else if(*res.choice == 3) {
      return std::move(res[0][1].result);
    } else {
      return std::move(res[0].result);
    }
    assert(false);
  };
  mPegParser["IfExpression"] << "'if' Expression ScopeExpression ('else' 'if' Expression ScopeExpression)* ('else' ScopeExpression)?" >> [] (peg::MatchInfo& res) -> peg::Any {
    IfExpressionChildList list;
    list.push_back(std::make_pair(
        up<ExpressionNode>{res[1].result.move<ExpressionNode*>()},
        up<ScopeNode>{res[2].result.move<ScopeNode*>()}));
    for(auto& elseIf : res[3].subs) {
      list.push_back(std::make_pair(
          up<ExpressionNode>{elseIf[2].result.move<ExpressionNode*>()},
          up<ScopeNode>{elseIf[3].result.move<ScopeNode*>()}));
    }
    up<ScopeNode> elseBody;
    if(!res[4].subs.empty()) {
      elseBody.reset(res[4][0][1].result.move<ScopeNode*>());
    }
    return IfExpressionNode{std::move(list), std::move(elseBody)};
  };
  mPegParser["FunctionCallExpression"] << "Identifier '(' ParameterListWithoutDatatype ')'" >> [] (peg::MatchInfo& res) -> peg::Any {
    return FunctionCallExpressionNode{
      up<IdentifierNode>{res[0].result.move<IdentifierNode*>()},
      up<ParameterListNodeWithoutDatatypes>{res[2].result.move<ParameterListNodeWithoutDatatypes*>()}};
  };
  mPegParser["ParameterListWithoutDatatype"] << "ParameterListWithoutDatatypeRec?" >> [] (peg::MatchInfo& res) -> peg::Any {
    if(res.subs.empty())
      return ParameterListNodeWithoutDatatypes{{}};
    return ParameterListNodeWithoutDatatypes{res[0].result.moveValue<std::vector<up<ExpressionNode>>>()};
  };
  mPegParser["ParameterListWithoutDatatypeRec"] << "Expression (',' ParameterListWithoutDatatypeRec)?" >> [] (peg::MatchInfo& res) {
    std::vector<up<ExpressionNode>> params;
    params.emplace_back(up<ExpressionNode>{res[0].result.move<ExpressionNode*>()});
    // recursive child
    if(!res[1].subs.empty()) {
      auto childParams = res[1][0][1].result.moveValue<std::vector<up<ExpressionNode>>>();
      params.insert(params.end(), std::move_iterator(childParams.begin()), std::move_iterator(childParams.end()));
    }
    return params;
  };
}

up<ASTNode> Parser::parse(std::string code) const {
  Stopwatch stopwatch{"Parsing the code"};
  auto ret = mPegParser.parse("Start", std::move(code));
  if(ret.first.index() == 0)
    return up<ASTNode>{std::get<0>(ret.first).moveMatchInfo().result.move<ASTNode*>()};

  std::cerr << peg::errorsToString(std::get<1>(ret.first), ret.second);
  return nullptr;
}
}