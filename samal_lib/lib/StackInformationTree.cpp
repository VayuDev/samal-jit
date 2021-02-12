#include "samal_lib/StackInformationTree.hpp"
namespace samal {

StackInformationTree::StackInformationTree(int32_t startIp, int32_t totalStackSize, IsAtPopInstruction isAtPop)
: mStartIp(startIp), mTotalStackSize(totalStackSize), mIsAtPopInstruction(isAtPop) {
}
StackInformationTree::StackInformationTree(int32_t startIp, int32_t totalStackSize, std::string name, Datatype datatype, StorageType storageType)
: mStartIp(startIp), mTotalStackSize(totalStackSize), mVariable(VariableEntry{ .name = std::move(name), .datatype = std::move(datatype), .storageType = storageType }) {
}
StackInformationTree* StackInformationTree::addChild(up<StackInformationTree> child) {
    if(mLastChild) {
        child->mPrevSibling = std::move(mLastChild);
    }
    mLastChild = std::move(child);
    mLastChild->mParent = this;
    return mLastChild.get();
}
StackInformationTree* StackInformationTree::getParent() {
    return mParent;
}
StackInformationTree* StackInformationTree::getBestNodeForIp(int32_t ip) {
    if(mStartIp <= ip && !mLastChild) {
        return this;
    }
    if(mStartIp > ip) {
        return nullptr;
    }
    StackInformationTree* current = mLastChild.get();
    std::vector<StackInformationTree*> children;
    while(current) {
        children.push_back(current);
        current = current->mPrevSibling.get();
    }
    std::reverse(children.begin(), children.end());
    StackInformationTree* childBestNode = mLastChild.get();
    for(auto& child : children) {
        auto ret = child->getBestNodeForIp(ip);
        if(!ret) {
            break;
        }
        childBestNode = ret;
    }
    return childBestNode;
}
StackInformationTree* StackInformationTree::getPrevSibling() {
    return mPrevSibling.get();
}
int32_t StackInformationTree::getStackSize() const {
    return mTotalStackSize;
}
bool StackInformationTree::isAtPopInstruction() const {
    return mIsAtPopInstruction == IsAtPopInstruction::Yes;
}

}