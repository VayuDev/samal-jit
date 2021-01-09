#pragma once
#include "AST.hpp"
#include "Forward.hpp"
#include "Util.hpp"
#include <map>
#include <vector>

namespace samal {

class DatatypeCompleter {
private:
    class ScopeChecker final {
    public:
        explicit ScopeChecker(DatatypeCompleter& completer, const std::string& moduleName);
        ~ScopeChecker();

    private:
        DatatypeCompleter& mCompleter;
    };

public:
    void declareModules(std::vector<up<ModuleRootNode>>&);
    void complete(up<ModuleRootNode>& root);
    ScopeChecker openScope(const std::string& moduleName = "");
    void declareVariable(const std::string& name, Datatype type);
    void declareVariableNonOverrideable(const std::string& name, Datatype type);
    void saveModule(std::string name);
    [[nodiscard]] std::pair<Datatype, int32_t> getVariableType(const std::vector<std::string>& name) const;

private:
    void declareVariable(const std::string& name, Datatype type, bool overrideable);
    void pushScope(const std::string&);
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
    std::string mCurrentModule;
};

}
