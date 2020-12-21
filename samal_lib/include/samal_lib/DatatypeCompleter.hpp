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
  void declareVariable(const std::string& name, Datatype type);
  void declareVariableNonOverrideable(const std::string& name, Datatype type);
  void saveModule(std::string name);
  [[nodiscard]] std::pair<Datatype, int32_t> getVariableType(const std::string& name) const;
 private:
  void declareVariable(const std::string& name, Datatype type, bool overrideable);
  void pushScope();
  void popScope();
  struct VariableDeclaration {
    Datatype type;
    int32_t id;
    bool overrideable;
  };
  using ScopeFrame = std::map<std::string, VariableDeclaration>;
  std::map<std::string, ScopeFrame> mModules;
  std::vector<ScopeFrame> mScope;
  int32_t mIdCounter = 0;
};

}
