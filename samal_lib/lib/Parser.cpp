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
        res.subs.at(1).result.moveValue<std::string>(),
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
    params.emplace_back(ParameterListNode::Parameter{res[0].result.moveValue<std::string>(), res[2].result.moveValue<Datatype>()});
    // recursive child
    if(!res[3].subs.empty()) {
      auto childParams = res[3][0][1].result.moveValue<std::vector<ParameterListNode::Parameter>>();
      params.insert(params.end(), std::move_iterator(childParams.begin()), std::move_iterator(childParams.end()));
    }
    return params;
  };
  mPegParser["Identifier"] << "[a-zA-Z]+ ~nws~(~nws~[\\da-zA-Z])*" >> [] (peg::MatchInfo& r) {
    return std::string{r.startTrimmed(), r.endTrimmed()};
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
  mPegParser["Expression"] << "LineExpression | ScopeExpression" >> [] (peg::MatchInfo& res) -> peg::Any {
    return std::move(res[0].result);
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
  mPegParser["LiteralExpression"] << "[\\d]+ | '(' Expression ')' | ScopeExpression" >> [] (peg::MatchInfo& res) -> peg::Any {
    int32_t val;
    if(*res.choice == 0) {
      std::from_chars(res.startTrimmed(), res.endTrimmed(), val);
      return LiteralInt32Node{val};
    } else if(*res.choice == 1) {
      return std::move(res[0][1].result);
    } else {
      return std::move(res[0].result);
    }
    assert(false);
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