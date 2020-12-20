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
void DatatypeCompleter::declareVariable(std::string name, Datatype type) {
  mScope.at(mScope.size() - 1).erase(name);
  auto insertResult = mScope.at(mScope.size() - 1).emplace(std::make_pair(name, std::make_pair(std::move(type), mIdCounter++)));
  assert(insertResult.second);
}
std::pair<Datatype, int32_t> DatatypeCompleter::getVariableType(const std::string& name) const {
  for(ssize_t i = mScope.size() - 1 ; i >= 0; --i) {
    auto value = mScope.at(i).find(name);
    if(value != mScope.at(i).end()) {
      return value->second;
    }
  }
  for(auto& [_, module]: mModules) {
    auto value = module.find(name);
    if(value != module.end()) {
      return value->second;
    }
  }
  throw std::runtime_error{"Couldn't find a variable called " + name};
}
void DatatypeCompleter::saveModule(std::string name) {
  assert(mScope.size() == 1);
  mModules.emplace(std::make_pair(std::move(name), mScope.at(0)));
}

}