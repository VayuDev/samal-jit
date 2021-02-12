#pragma once
#include "Datatype.hpp"
#include "Util.hpp"
#include <string>
#include <vector>

namespace samal {

enum class StorageType {
    Local,
    Parameter
};

class StackInformationTree {
public:
    // If we hit a pop instruction while traversing the tree later,
    // this means that all previous variables have already been popped
    // so we need to skip them all.
    enum class IsAtPopInstruction {
        Yes,
        No
    };
    StackInformationTree(int32_t startIp, int32_t totalStackSize, IsAtPopInstruction);
    StackInformationTree(int32_t startIp, int32_t totalStackSize, std::string name, Datatype type, StorageType);
    StackInformationTree* addChild(up<StackInformationTree> child);
    StackInformationTree* getParent();
    StackInformationTree* getBestNodeForIp(int32_t ip);
    StackInformationTree* getPrevSibling();
    [[nodiscard]] int32_t getStackSize() const;
    [[nodiscard]] bool isAtPopInstruction() const;
    [[nodiscard]] inline const auto& getVarEntry() const {
        return mVariable;
    }

private:
    struct VariableEntry {
        std::string name;
        Datatype datatype;
        StorageType storageType;
    };
    StackInformationTree* mParent{ nullptr };
    up<StackInformationTree> mPrevSibling;
    int32_t mStartIp{ -1 };
    int32_t mTotalStackSize{ 0 };
    std::optional<VariableEntry> mVariable;
    up<StackInformationTree> mLastChild;
    IsAtPopInstruction mIsAtPopInstruction{ IsAtPopInstruction::No };
};

}