#include "samal_lib/Parser.hpp"
#include "samal_lib/AST.hpp"
#include <iostream>

namespace samal {

Parser::Parser() {
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
  mPegParser["FunctionDeclaration"] << "'fn' Identifier '(' ParameterList ')' '->' Datatype Scope" >> [] (peg::MatchInfo& res) -> peg::Any {
    return FunctionDeclarationNode(
        res.subs.at(1).result.moveValue<std::string>(),
            up<ParameterListNode>{res.subs.at(3).result.move<ParameterListNode*>()},
            res.subs.at(6).result.moveValue<Datatype>(),
            nullptr);
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
  mPegParser["Identifier"] << "[a-z,A-Z]+" >> [] (peg::MatchInfo& r) {
    return std::string{r.startTrimmed(), r.endTrimmed()};
  };
  mPegParser["Datatype"] << "'i32' | [a-z,A-Z]+" >> [] (peg::MatchInfo& res) -> peg::Any {
    switch(*res.choice) {
      case 0:
        return Datatype{DatatypeCategory::i32};
      case 1:
        return Datatype{DatatypeCategory::undetermined_identifier};
      default:
        assert(false);
    }
  };
  mPegParser["Scope"] << "'{' Expression* '}'" >> [] (peg::MatchInfo&) {
    return peg::Any{};
  };
  mPegParser["Expression"] << "'0'" >> [] (peg::MatchInfo&) {
    return peg::Any{};
  };
}

up<ASTNode> Parser::parse(std::string code) const {
  auto ret = mPegParser.parse("Start", std::move(code));
  if(ret.first.index() == 0)
    return up<ASTNode>{std::get<0>(ret.first).moveMatchInfo().result.move<ASTNode*>()};

  std::cerr << peg::errorsToString(std::get<1>(ret.first), ret.second);
  return nullptr;
}
}