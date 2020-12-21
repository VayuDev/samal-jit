#include <cassert>
#include "samal_lib/DatatypeCompleter.hpp"

namespace samal {

DatatypeCompleter::ScopeChecker::ScopeChecker(DatatypeCompleter &completer)
: mCompleter(completer) {
  mCompleter.pushScope();
}
DatatypeCompleter::ScopeChecker::~ScopeChecker() {
  mCompleter.popScope();
}
void DatatypeCompleter::declareModules(std::vector<up<ModuleRootNode>>& roots) {
  for(auto& decl: roots) {
    decl->declareShallow(*this);
  }
}
void DatatypeCompleter::complete(up<ModuleRootNode> &root) {
  root->completeDatatype(*this);
}
DatatypeCompleter::ScopeChecker DatatypeCompleter::openScope() {
  return DatatypeCompleter::ScopeChecker(*this);
}
void DatatypeCompleter::pushScope() {
  mScope.emplace_back();
}
void DatatypeCompleter::popScope() {
  mScope.erase(mScope.end() - 1);
}
void DatatypeCompleter::declareVariable(const std::string& name, Datatype type) {
  declareVariable(name, std::move(type), true);
}
void DatatypeCompleter::declareVariableNonOverrideable(const std::string& name, Datatype type) {
  declareVariable(name, std::move(type), false);
}
void DatatypeCompleter::declareVariable(const std::string &name, Datatype type, bool overrideable) {
  auto& currentScope = mScope.at(mScope.size() - 1);
  auto varAlreadyInScope = currentScope.find(name);
  if(varAlreadyInScope != currentScope.end() && !varAlreadyInScope->second.overrideable) {
    throw std::runtime_error{"Overriding non-overrideable variable named '" + name + "'"};
  }
  currentScope.erase(varAlreadyInScope->first);
  auto insertResult = currentScope.emplace(std::make_pair(name, VariableDeclaration{.type = std::move(type), .id = mIdCounter++, .overrideable = overrideable}));
  assert(insertResult.second);
}
std::pair<Datatype, int32_t> DatatypeCompleter::getVariableType(const std::string& name) const {
  for(ssize_t i = mScope.size() - 1 ; i >= 0; --i) {
    auto value = mScope.at(i).find(name);
    if(value != mScope.at(i).end()) {
      return std::make_pair(value->second.type, value->second.id);
    }
  }
  for(auto& [_, module]: mModules) {
    auto value = module.find(name);
    if(value != module.end()) {
      return std::make_pair(value->second.type, value->second.id);
    }
  }
  throw std::runtime_error{"Couldn't find a variable called " + name};
}
void DatatypeCompleter::saveModule(std::string name) {
  assert(mScope.size() == 1);
  mModules.emplace(std::make_pair(std::move(name), mScope.at(0)));
}

}