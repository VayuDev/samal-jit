#include "samal_lib/StackInformationTree.hpp"
namespace samal {

StackInformationTree::StackInformationTree(int32_t startIp, int32_t totalStackSize, IsAtPopInstruction isAtPop)
: mStartIp(startIp), mTotalStackSize(totalStackSize), mIsAtPopInstruction(isAtPop) {
}
StackInformationTree::StackInformationTree(int32_t startIp, int32_t totalStackSize, std::string name, Datatype datatype, StorageType storageType)
: mStartIp(startIp), mTotalStackSize(totalStackSize), mVariable(VariableEntry{ .name = std::move(name), .datatype = std::move(datatype), .storageType = storageType }) {
}
StackInformationTree* StackInformationTree::addChild(up<StackInformationTree> child) {
    if(!mChildren.empty()) {
        child->mPrevSibling = mChildren.back().get();
    }
    child->mParent = this;
    mChildren.emplace_back(std::move(child));
    return mChildren.back().get();
}
StackInformationTree* StackInformationTree::getBestNodeForIp(int32_t ip) {
    if(mStartIp <= ip && mChildren.empty()) {
        return this;
    }
    if(mStartIp > ip) {
        return nullptr;
    }
    StackInformationTree* childBestNode = mChildren.back().get();
    for(auto it = mChildren.begin(); it != mChildren.end(); ++it) {
        auto ret = (*it)->getBestNodeForIp(ip);
        if(!ret) {
            break;
        }
        childBestNode = ret;
    }
    return childBestNode;
}
int32_t StackInformationTree::getStackSize() const {
    return mTotalStackSize;
}

}