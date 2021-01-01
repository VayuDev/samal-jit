#include <cassert>
#include "samal_lib/DatatypeCompleter.hpp"

namespace samal {

DatatypeCompleter::ScopeChecker::ScopeChecker(DatatypeCompleter &completer, const std::string& moduleName)
: mCompleter(completer) {
  mCompleter.pushScope(moduleName);
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
DatatypeCompleter::ScopeChecker DatatypeCompleter::openScope(const std::string& moduleName) {
  return DatatypeCompleter::ScopeChecker(*this, moduleName);
}
void DatatypeCompleter::pushScope(const std::string& name) {
  if(!name.empty()) {
    mCurrentModule = name;
  }
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
  if(name.find('.') != std::string::npos) {
    throw std::runtime_error{"The '.' character is not allowed in variable declarations"};
  }
  auto& currentScope = mScope.at(mScope.size() - 1);
  auto varAlreadyInScope = currentScope.find(name);
  if(varAlreadyInScope != currentScope.end()) {
    if(!varAlreadyInScope->second.overrideable) {
      throw std::runtime_error{"Overriding non-overrideable variable named '" + name + "'"};
    }
    currentScope.erase(varAlreadyInScope->first);
  }

  auto insertResult = currentScope.emplace(std::make_pair(name, VariableDeclaration{.type = std::move(type), .id = mIdCounter++, .overrideable = overrideable}));
  assert(insertResult.second);
}
std::pair<Datatype, int32_t> DatatypeCompleter::getVariableType(const std::vector<std::string>& name) const {
  std::vector<std::string> fullPath = name;
  assert(fullPath.size() == 1 || fullPath.size() == 2);
  if(name.size() == 1) {
    for(ssize_t i = mScope.size() - 1 ; i >= 0; --i) {
      auto value = mScope.at(i).find(name.at(0));
      if(value != mScope.at(i).end()) {
        return std::make_pair(value->second.type, value->second.id);
      }
    }
    // prepend current module name
    fullPath = name;
    fullPath.emplace(fullPath.begin(), mCurrentModule);
  }
  auto module = mModules.find(fullPath.at(0));
  if(module != mModules.end()) {
    auto& moduleDeclarations = module->second;
    auto value = moduleDeclarations.find(fullPath.at(1));
    if(value != moduleDeclarations.end()) {
      return std::make_pair(value->second.type, value->second.id);
    }
  }
  throw std::runtime_error{"Couldn't find a variable called " + concat(name)};
}
void DatatypeCompleter::saveModule(std::string name) {
  assert(mScope.size() == 1);
  mModules.emplace(std::make_pair(std::move(name), mScope.at(0)));
}

}