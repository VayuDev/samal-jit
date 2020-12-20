#pragma once
#include "Forward.hpp"
#include "Util.hpp"
#include <vector>
#include <map>
#include "AST.hpp"

namespace samal {

class DatatypeCompleter {
 private:

  class ScopeChecker final {
   public:
    explicit ScopeChecker(DatatypeCompleter& completer);
    ~ScopeChecker();
   private:
    DatatypeCompleter& mCompleter;
  };

 public:
  void declareModules(std::vector<up<ModuleRootNode>>&);
  void complete(up<ModuleRootNode>& root);
  ScopeChecker openScope();
  void declareVariable(std::string name, Datatype type);
  void saveModule(std::string name);
  [[nodiscard]] std::pair<Datatype, int32_t> getVariableType(const std::string& name) const;
 private:
  void pushScope();
  void popScope();
  using ScopeFrame = std::map<std::string, std::pair<Datatype, int32_t>>;
  std::map<std::string, ScopeFrame> mModules;
  std::vector<ScopeFrame> mScope;
  int32_t mIdCounter = 0;
};

}
