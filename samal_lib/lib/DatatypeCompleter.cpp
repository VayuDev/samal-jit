#include "samal_lib/DatatypeCompleter.hpp"
#include <cassert>

namespace samal {

DatatypeCompleter::ScopeChecker::ScopeChecker(DatatypeCompleter& completer, const std::string& moduleName)
: mCompleter(completer) {
    mCompleter.pushScope(moduleName);
}
DatatypeCompleter::ScopeChecker::~ScopeChecker() {
    mCompleter.popScope();
}
void DatatypeCompleter::declareModules(std::vector<up<ModuleRootNode>>& roots) {
    mTemplateInstantiationInfo = {};
    for(auto& module : roots) {
        module->declareShallow(*this);
        for(auto& decl: module->getDeclarations()) {
            mIdentifierToDeclaration.emplace(decl->getIdentifier(), decl.get());
            mDeclarationToModule.emplace(decl.get(), module.get());
        }
    }
}
std::map<const IdentifierNode*, TemplateInstantiationInfo> DatatypeCompleter::complete(up<ModuleRootNode>& root) {
    for(auto& decl: root->getDeclarations()) {
        if(!decl->hasTemplateParameters()) {
            mDeclarationsToComplete.push_back(DeclarationToComplete{.declaration = decl.get(), .templateInstantiationInfo{}});
        }
    }
    while(!mDeclarationsToComplete.empty()) {
        // open the scope of the module of the current declaration
        auto scope = this->openScope(mDeclarationToModule.at(mDeclarationsToComplete.at(0).declaration)->getModuleName());

        mCurrentTemplateReplacementsMap = createTemplateParamMap(mDeclarationsToComplete.at(0).declaration->getTemplateParameterVector(), mDeclarationsToComplete.at(0).templateInstantiationInfo);
        mDeclarationsToComplete.at(0).declaration->completeDatatype(*this);
        mDeclarationsToComplete.erase(mDeclarationsToComplete.cbegin());
        mCurrentTemplateReplacementsMap.reset();
    }
    return std::move(mTemplateInstantiationInfo);
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
void DatatypeCompleter::declareVariable(const IdentifierNode& name, Datatype type, std::vector<Datatype> templateParameters) {
    declareVariable(name, std::move(type), std::move(templateParameters), true);
}
void DatatypeCompleter::declareFunction(const IdentifierNode& identifierNode, Datatype type) {
    declareVariable(identifierNode, std::move(type), identifierNode.getTemplateParameters(), false);
    if(!identifierNode.getTemplateParameters().empty()) {
        bool inserted = mTemplateInstantiationInfo.emplace(&identifierNode, std::vector<std::vector<Datatype>>{}).second;
        assert(inserted);
    }
}
void DatatypeCompleter::declareVariable(const IdentifierNode& identifier, Datatype type, std::vector<Datatype> templateParameters, bool overrideable) {
    // only perform replacements if the type has template parameters and we are inside of a template function; otherwise it could also be the declaration
    if(type.hasUndeterminedTemplateTypes() && mCurrentTemplateReplacementsMap) {
        type = performTemplateReplacement(type);
    }
    if(identifier.getName().find('.') != std::string::npos) {
        throw std::runtime_error{ "The '.' character is not allowed in variable declarations" };
    }
    auto& currentScope = mScope.at(mScope.size() - 1);
    auto varAlreadyInScope = currentScope.find(identifier.getName());
    if(varAlreadyInScope != currentScope.end()) {
        if(!varAlreadyInScope->second.overrideable) {
            throw std::runtime_error{ "Overriding non-overrideable variable named '" + identifier.getName() + "'" };
        }
        currentScope.erase(varAlreadyInScope->first);
    }
    auto insertResult = currentScope.emplace(std::make_pair(identifier.getName(),
        VariableDeclaration{ .identifier = &identifier, .type = std::move(type), .templateParameters = std::move(templateParameters), .id = mIdCounter++, .overrideable = overrideable }));
    assert(insertResult.second);
}
std::pair<Datatype, IdentifierId> DatatypeCompleter::getVariableType(const IdentifierNode& identifier) {
    const auto& templateParameters = identifier.getTemplateParameters();
    const auto& name = identifier.getNameSplit();
    auto returnFoundVariableDeclaration = [&] (const VariableDeclaration& value) {
        // just determine the identifier if it's not a template type
        if(value.templateParameters.empty()) {
            return std::make_pair(value.type, IdentifierId{ .variableId = value.id, .templateId = 0 });
        }
        // we have a template type, so it's going to get messy...

        // don't add this call if it's just the function declaration e.g. fib<T>
        bool shouldAddToInstantiatedTemplates = true;
        for(auto& usedTemplateParam: templateParameters) {
            if(usedTemplateParam.hasUndeterminedTemplateTypes()) {
                shouldAddToInstantiatedTemplates = false;
            }
        }
        int32_t templateId = -1;
        if(shouldAddToInstantiatedTemplates) {
            // check if this version is already instantiated, and, if not, at it to the list. Determine the templateId this way
            auto& templateInstantiations = mTemplateInstantiationInfo.at(value.identifier);
            std::optional<int32_t> templateIdOrNot;
            for(size_t i = 0; i < templateInstantiations.size(); ++i) {
                if(templateInstantiations.at(i) == templateParameters) {
                    templateIdOrNot = i;
                }
            }
            if(!templateIdOrNot.has_value()) {
                // we need to add it to the list.
                // Also add it to this list of template declarations to complete to check if it's fine.
                templateInstantiations.push_back(templateParameters);
                templateIdOrNot = templateInstantiations.size() - 1;
                mDeclarationsToComplete.push_back(DeclarationToComplete{.declaration = mIdentifierToDeclaration.at(value.identifier), .templateInstantiationInfo = templateParameters});
            }
            templateId = *templateIdOrNot;
        }
        return std::make_pair(value.type.completeWithTemplateParameters(createTemplateParamMap(value.templateParameters, templateParameters)), IdentifierId{ .variableId = value.id, .templateId = templateId });
    };

    std::vector<std::string> fullPath = name;
    assert(fullPath.size() == 1 || fullPath.size() == 2);
    if(name.size() == 1) {
        for(ssize_t i = mScope.size() - 1; i >= 0; --i) {
            auto value = mScope.at(i).find(name.at(0));
            if(value != mScope.at(i).end()) {
                return returnFoundVariableDeclaration(value->second);
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
            return returnFoundVariableDeclaration(value->second);
        }
    }
    throw std::runtime_error{ "Couldn't find a variable called " + concat(name) + " (" + concat(fullPath) + ")" };
}
void DatatypeCompleter::saveModule(std::string name) {
    assert(mScope.size() == 1);
    mModules.emplace(std::make_pair(std::move(name), mScope.at(0)));
}
Datatype DatatypeCompleter::performTemplateReplacement(const Datatype& source) {
    assert(mCurrentTemplateReplacementsMap);
    if(!source.hasUndeterminedTemplateTypes())
        return source;
    return source.completeWithTemplateParameters(*mCurrentTemplateReplacementsMap);
}

}