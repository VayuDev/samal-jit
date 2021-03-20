#include "samal_lib/GC.hpp"
#include "samal_lib/Program.hpp"
#include "samal_lib/VM.hpp"
#include <algorithm>
#include <sys/mman.h>

#ifndef MAP_NORESERVE
#define MAP_NORESERVE 0
#endif
#define INITIAL_HEAP_SIZE (1024 * 1024)

namespace samal {

GC::Region::Region(size_t len) {
    if(len > 0) {
        base = (uint8_t*)mmap(nullptr, len, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON | MAP_NORESERVE, -1, 0);
        assert(base);
        size = len;
    }
}
GC::Region::~Region() {
    if(base) {
        if(munmap(base, size)) {
            perror("munmap()");
        }
    }
}
GC::Region::Region(GC::Region&& other) noexcept {
    operator=(std::move(other));
}
GC::Region& GC::Region::operator=(GC::Region&& other) noexcept {
    base = other.base;
    size = other.size;
    offset = other.offset;
    other.offset = 0;
    other.size = 0;
    other.base = nullptr;
    return *this;
}

GC::GC(VM& vm)
: mVM(vm) {
    mRegions[0] = Region{ INITIAL_HEAP_SIZE };
    mRegions[1] = Region{ INITIAL_HEAP_SIZE };
}
uint8_t* GC::alloc(int32_t size) {
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
bool GC::copyToOther(uint8_t** ptr, size_t len) {
    auto maybePointerAddress = mMovedPointers.find(*ptr);
    if(maybePointerAddress == mMovedPointers.end()) {
        uint8_t* newPtr = getOtherRegion().top();
        memcpy(newPtr, *ptr, len);
        getOtherRegion().offset += len;
        mMovedPointers.emplace(*ptr, newPtr);
        *ptr = newPtr;
        return true;
    } else {
        *ptr = maybePointerAddress->second;
        return false;
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
        searchForPtrs(*ptrToCurrent + 8, type.getListContainedType(), ScanningHeapOrStack::Heap);
        if(copyToOther(ptrToCurrent, containedTypeSize + 8)) {
            searchForPtrs(*ptrToCurrent, type, ScanningHeapOrStack::Heap);
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
        int32_t capturedLambdaTypesId;
        memcpy(&capturedLambdaTypesId, lambdaPtr + 8, 4);
        searchForPtrs(lambdaPtr + 16, mVM.getProgram().auxiliaryDatatypes.at(capturedLambdaTypesId), ScanningHeapOrStack::Heap);

        int32_t sizeOfLambda = -1;
        memcpy(&sizeOfLambda, lambdaPtr, 4);
        sizeOfLambda += 16;
        copyToOther((uint8_t**)ptr, sizeOfLambda);
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
        searchForPtrs(*(uint8_t**)ptr, type.getPointerBaseType(), ScanningHeapOrStack::Heap);
        copyToOther((uint8_t**)ptr, type.getPointerBaseType().getSizeOnStack());
        break;
    }
    default:
        assert(false);
    }
}
void GC::requestCollection() {
    callsSinceLastRun++;
    if(callsSinceLastRun > 400000) {
        performGarbageCollection();
        callsSinceLastRun = 0;
    }
}
GC::Region& GC::getActiveRegion() {
    return mRegions[mActiveRegion];
}
GC::Region& GC::getOtherRegion() {
    return mRegions[!mActiveRegion];
}
}
