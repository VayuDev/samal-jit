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
VM Pipeline::compile(VMParameters params) {
    samal::Compiler compiler{ mModules, std::move(mNativeFunctions) };
    auto program = compiler.compile();
    std::cout << program.disassemble() << "\n";
    return samal::VM{ std::move(program), params };
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
            auto declAsStructDecl = dynamic_cast<StructDeclarationNode*>(decl.get());
            if(declAsStructDecl) {
                auto fullName = module->getModuleName() + "." + declAsStructDecl->getIdentifier()->getName();
                Datatype type = Datatype::createStructType(fullName, declAsStructDecl->getFields(), declAsStructDecl->getTemplateParameterVector());
                replacementMap.emplace(fullName, std::make_pair(std::move(type), TemplateParamOrUserType::UserType));
            }
            auto declAsEnumDecl = dynamic_cast<EnumDeclarationNode*>(decl.get());
            if(declAsEnumDecl) {
                auto fullName = module->getModuleName() + "." + declAsEnumDecl->getIdentifier()->getName();
                Datatype type = Datatype::createEnumType(fullName, declAsEnumDecl->getFields(), declAsEnumDecl->getTemplateParameterVector());
                replacementMap.emplace(fullName, std::make_pair(std::move(type), TemplateParamOrUserType::UserType));
            }
        }
    }
    return parsedType.completeWithTemplateParameters(replacementMap, {}, allowIncompleteTypes);
}

}