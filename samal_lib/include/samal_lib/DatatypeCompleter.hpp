#pragma once
#include "AST.hpp"
#include "Forward.hpp"
#include "Util.hpp"
#include <map>
#include <vector>
#include "IdentifierId.hpp"

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
    std::map<const IdentifierNode*, TemplateInstantiationInfo> complete(up<ModuleRootNode>& root);
    ScopeChecker openScope(const std::string& moduleName = "");
    void declareVariable(const IdentifierNode& name, Datatype type, std::vector<Datatype> templateParameters);
    void declareFunction(const IdentifierNode& identifier, Datatype type);
    void saveModule(std::string name);
    [[nodiscard]] std::pair<Datatype, IdentifierId> getVariableType(const std::vector<std::string>& identifierTemplateInfo, const std::vector<Datatype>& templateParameters);

private:
    void declareVariable(const IdentifierNode& name, Datatype type, std::vector<Datatype> templateParameters, bool overrideable);
    void pushScope(const std::string&);
    void popScope();
    struct VariableDeclaration {
        const IdentifierNode* identifier{ nullptr };
        Datatype type;
        std::vector<Datatype> templateParameters;
        int32_t id{ -1 };
        bool overrideable{ false };
    };
    using ScopeFrame = std::map<std::string, VariableDeclaration>;
    std::map<std::string, ScopeFrame> mModules;
    std::vector<ScopeFrame> mScope;
    int32_t mIdCounter = 0;
    std::string mCurrentModule;
    std::map<const IdentifierNode*, TemplateInstantiationInfo> mTemplateInstantiationInfo;
};

}
