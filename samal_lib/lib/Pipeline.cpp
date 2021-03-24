#include "samal_lib/Pipeline.hpp"
#include "samal_lib/AST.hpp"
#include "samal_lib/Compiler.hpp"
#include "samal_lib/Parser.hpp"
#include "samal_lib/Util.hpp"
#include <cstdio>
#include <filesystem>

namespace samal {

Pipeline::Pipeline() {
    mParser = std::make_unique<Parser>();
}
Pipeline::~Pipeline() {
}
void Pipeline::addFile(const std::string& path) {
    auto file = fopen(path.c_str(), "r");
    if(!file) {
        throw std::runtime_error{ "Couldn't open " + path };
    }
    DestructorWrapper destructor{
        [&] {
            fclose(file);
        }
    };
    fseek(file, 0, SEEK_END);
    auto fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);
    std::string fileContents;
    fileContents.resize(fileSize);
    fread((char*)fileContents.c_str(), 1, fileSize, file);
    std::filesystem::path pathObj{ path };
    addFileFromMemory(pathObj.stem(), std::move(fileContents));
}
void Pipeline::addFileFromMemory(std::string moduleName, std::string fileContents) {
    auto [module, tokenizer] = mParser->parse(std::move(moduleName), std::move(fileContents));
    mModules.emplace_back(std::move(module));
    mTokenizers.emplace_back(std::move(tokenizer));
}
VM Pipeline::compile() {
    samal::Compiler compiler{ mModules, std::move(mNativeFunctions) };
    return samal::VM{ compiler.compile() };
}
void Pipeline::addNativeFunction(NativeFunction function) {
    mNativeFunctions.emplace_back(std::move(function));
}
Datatype Pipeline::type(const std::string& typeString) {
    return parseTypeInternal(typeString, Datatype::AllowIncompleteTypes::No);
}
Datatype Pipeline::incompleteType(const std::string& typeString) {
    return parseTypeInternal(typeString, Datatype::AllowIncompleteTypes::Yes);
}
Datatype Pipeline::parseTypeInternal(const std::string& typeString, Datatype::AllowIncompleteTypes allowIncompleteTypes) {
    auto [parsedType, tokenizer] = mParser->parseDatatype(typeString);
    UndeterminedIdentifierReplacementMap replacementMap;
    for(auto& module: mModules) {
        for(auto& decl: module->getDeclarations()) {
            Datatype type;
            auto declAsStructDecl = dynamic_cast<StructDeclarationNode*>(decl.get());
            auto fullName = module->getModuleName() + "." + decl->getIdentifier()->getName();
            if(declAsStructDecl) {
                type = Datatype::createStructType(fullName, declAsStructDecl->getFields(), declAsStructDecl->getTemplateParameterVector());
            }
            auto declAsEnumDecl = dynamic_cast<EnumDeclarationNode*>(decl.get());
            if(declAsEnumDecl) {
                type = Datatype::createEnumType(fullName, declAsEnumDecl->getFields(), declAsEnumDecl->getTemplateParameterVector());
            }
            if(type.getCategory() != DatatypeCategory::invalid) {
                replacementMap.emplace(fullName, std::make_pair(std::move(type), TemplateParamOrUserType::UserType));
            }
        }
    }
    return parsedType.completeWithTemplateParameters(replacementMap, {}, allowIncompleteTypes);
}

}