#pragma once
#include "Forward.hpp"
#include "VM.hpp"
#include "peg_parser/PegForward.hpp"
#include <string>

namespace samal {

class Pipeline final {
public:
    Pipeline();
    ~Pipeline();
    void addFile(const std::string& path);
    void addFileFromMemory(std::string moduleName, std::string fileContents);
    void addNativeFunction(NativeFunction function);
    samal::VM compile();

    Datatype type(const std::string& typeString);
    Datatype incompleteType(const std::string& typeString);

private:
    Datatype parseTypeInternal(const std::string& typeString, Datatype::AllowIncompleteTypes);
    up<samal::Parser> mParser;
    std::vector<up<samal::ModuleRootNode>> mModules;
    std::vector<peg::PegTokenizer> mTokenizers;
    std::vector<NativeFunction> mNativeFunctions;
};

}