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
  mPegParser["Declaration"] << "'fn' Identifier '(' ParameterList? ')' '->' Datatype '{' '0' '}' " >> [] (peg::MatchInfo&) -> peg::Any {
    return FunctionDeclarationNode();
  };
  mPegParser["ParameterList"] << "Identifier ':' Datatype (',' ParameterList)?" >> [] (peg::MatchInfo&) {
    return peg::Any{};
  };
  mPegParser["Identifier"] << "[a-z,A-Z]+" >> [] (peg::MatchInfo&) {
    return peg::Any{};
  };
  mPegParser["Datatype"] << "'i32' | [a-z,A-Z]+" >> [] (peg::MatchInfo&) {
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