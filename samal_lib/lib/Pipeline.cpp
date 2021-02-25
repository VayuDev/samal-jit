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

}