#include "samal_lib/GC.hpp"
#include "samal_lib/Program.hpp"
#include "samal_lib/VM.hpp"
#include <algorithm>
#include <sys/mman.h>

#ifndef MAP_NORESERVE
#define MAP_NORESERVE 0
#endif

namespace samal {

GC::Region::Region(size_t len) {
    if(len > 0) {
        base = (uint8_t*)malloc(len);
        assert(base);
        size = len;
    }
    offset = 0;
}
GC::Region::~Region() {
    if(base) {
        free(base);
        base = nullptr;
    }
}
GC::Region::Region(GC::Region&& other) noexcept {
    operator=(std::move(other));
}
GC::Region& GC::Region::operator=(GC::Region&& other) noexcept {
    if(base)
        free(base);
    base = other.base;
    size = other.size;
    offset = other.offset;
    other.offset = 0;
    other.size = 0;
    other.base = nullptr;
    return *this;
}

GC::GC(VM& vm, const VMParameters& params)
: mVM(vm) {
    mConfigFunctionsCallsPerGCRun = params.functionsCallsPerGCRun;
    mRegions[0] = Region{ static_cast<size_t>(params.initialHeapSize) };
    mRegions[1] = Region{ static_cast<size_t>(params.initialHeapSize) };
}
uint8_t* GC::alloc(int32_t size) {
    // ensure that every memory allocation is divisible by 2 (used for pointer tagging for lambdas/functions)
    if(size % 2 == 1) {
        size += 1;
    }
    if(mRegions[mActiveRegion].offset + size >= mRegions[mActiveRegion].size) {
        // not enough memory, add to temporary allocations
        mTemporaryAllocations.emplace_back(TemporaryAllocation{{new uint8_t[size], ArrayDeleter{}}, (size_t)size});
        return mTemporaryAllocations.back().data.get();
    }
    auto ptr = mRegions[mActiveRegion].top();
    mRegions[mActiveRegion].offset += size;
    return ptr;
}
void GC::performGarbageCollection() {
    //Stopwatch stopwatch{"GC"};
    //printf("Before size: %i\n", (int)mRegions[mActiveRegion].offset);
    getOtherRegion().offset = 0;
    if(!mTemporaryAllocations.empty() || getOtherRegion().size < getActiveRegion().size) {
        // our other region that we're copying into might be too small, so we resize it to prevent any potential problems
        size_t totalTemporarySize = 0;
        for(auto& alloc: mTemporaryAllocations) {
            totalTemporarySize += alloc.len;
        }
        //printf("Resizing heap to %i\n", (int)(getActiveRegion().size + totalTemporarySize));
        getOtherRegion() = Region{getActiveRegion().size + totalTemporarySize};
        //printf("Resizing heap due to temporary allocations to %zu\n", getOtherRegion().size);
    }
    mMovedPointers.clear();
    mVM.generateStacktrace([this](const uint8_t* ptr, const Datatype& type, const std::string& name) {
        searchForPtrs((uint8_t*)ptr, type, ScanningHeapOrStack::Stack);
    }, {});
    mActiveRegion = !mActiveRegion;
    mTemporaryAllocations.clear();
    //printf("After size: %i\n", (int)mRegions[mActiveRegion].offset);
}
std::pair<bool, uint8_t*> GC::copyToOther(uint8_t** ptr, size_t len) {
    auto maybePointerAddress = mMovedPointers.find(*ptr);
    if(maybePointerAddress == mMovedPointers.end()) {
        uint8_t* newPtr = getOtherRegion().top();
        assert(getOtherRegion().size >= getOtherRegion().offset + len);
        memcpy(newPtr, *ptr, len);
        getOtherRegion().offset += len;
        mMovedPointers.emplace(*ptr, newPtr);
        *ptr = newPtr;
        return std::make_pair(true, newPtr);
    } else {
        *ptr = maybePointerAddress->second;
        return std::make_pair(false, maybePointerAddress->second);
    }
}
void GC::searchForPtrs(uint8_t* ptr, const Datatype& type, ScanningHeapOrStack scanningHeapOrStack) {
    switch(type.getCategory()) {
    case DatatypeCategory::bool_:
    case DatatypeCategory::i32:
    case DatatypeCategory::i64:
    case DatatypeCategory::f64:
    case DatatypeCategory::char_:
    case DatatypeCategory::byte:
        break;
    case DatatypeCategory::tuple: {
        int32_t offset = type.getSizeOnStack();
        for(auto& element : type.getTupleInfo()) {
            offset -= element.getSizeOnStack();
            searchForPtrs(ptr + offset, element, scanningHeapOrStack);
        }
        break;
    }
    case DatatypeCategory::list: {
        auto** ptrToCurrent = (uint8_t**)ptr;
        if(*ptrToCurrent == nullptr)
            break;
        auto containedTypeSize = type.getListContainedType().getSizeOnStack();
        auto[copied, newPtr] = copyToOther(ptrToCurrent, containedTypeSize + 8);
        if(copied) {
            searchForPtrs(newPtr, type, ScanningHeapOrStack::Heap);
            searchForPtrs(newPtr + 8, type.getListContainedType(), ScanningHeapOrStack::Heap);
        }

        break;
    }
    case DatatypeCategory::function: {
        int32_t firstHalf;
        memcpy(&firstHalf, ptr, 4);
        if(firstHalf % 2 != 0) {
            // not lambda
            break;
        }
        // it's a lambda
        auto lambdaPtr = *(uint8_t**)ptr;
        assert(lambdaPtr);

        int32_t sizeOfLambda = -1;
        memcpy(&sizeOfLambda, lambdaPtr, 4);
        sizeOfLambda += 16;
        auto [copied, newPtr] = copyToOther((uint8_t**)ptr, sizeOfLambda);
        if(!copied)
            break;

        int32_t capturedLambdaTypesId;
        memcpy(&capturedLambdaTypesId, lambdaPtr + 8, 4);

        size_t offset = 0;
        const auto& helperTuple = mVM.getProgram().auxiliaryDatatypes.at(capturedLambdaTypesId).getTupleInfo();
        for(ssize_t i = 0; i < helperTuple.size(); ++i) {
            searchForPtrs(newPtr + 16 + offset, helperTuple.at(i), ScanningHeapOrStack::Heap);
            offset += helperTuple.at(i).getSizeOnStack();
        }

        break;
    }
    case DatatypeCategory::struct_: {
        int32_t offset = type.getSizeOnStack();
        for(auto& field : type.getStructInfo().fields) {
            auto fieldType = field.type.completeWithSavedTemplateParameters();
            offset -= fieldType.getSizeOnStack();
            searchForPtrs(ptr + offset, fieldType, scanningHeapOrStack);
        }
        break;
    }
    case DatatypeCategory::enum_: {
#ifdef x86_64_BIT_MODE
        int64_t selectedIndex;
        memcpy(&selectedIndex, ptr, 8);
#else
        int32_t selectedIndex;
        memcpy(&selectedIndex, ptr, 4);
#endif
        auto &selectedField = type.getEnumInfo().fields.at(selectedIndex);
        int32_t offset = type.getEnumInfo().getLargestFieldSizePlusIndex();
        for(auto& element : selectedField.params) {
            auto elementType = element.completeWithSavedTemplateParameters();
            offset -= elementType.getSizeOnStack();
            searchForPtrs(ptr + offset, elementType, scanningHeapOrStack);
        }
        break;
    }
    case DatatypeCategory::pointer: {
        auto[copied, newPtr] = copyToOther((uint8_t**)ptr, type.getPointerBaseType().getSizeOnStack());
        if(copied)
            searchForPtrs(newPtr, type.getPointerBaseType(), ScanningHeapOrStack::Heap);
        break;
    }
    default:
        assert(false);
    }
}
void GC::requestCollection() {
    mFunctionCallsSinceLastRun++;
    if(mFunctionCallsSinceLastRun > mConfigFunctionsCallsPerGCRun) {
        performGarbageCollection();
        mFunctionCallsSinceLastRun = 0;
    }
}
GC::Region& GC::getActiveRegion() {
    return mRegions[mActiveRegion];
}
GC::Region& GC::getOtherRegion() {
    return mRegions[!mActiveRegion];
}
}
