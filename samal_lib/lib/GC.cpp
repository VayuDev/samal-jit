#include "samal_lib/GC.hpp"
#include "samal_lib/Program.hpp"
#include "samal_lib/VM.hpp"
#include <algorithm>

#define HEAP_SIZE (512 * 1024 * 1024)

namespace samal {

GC::GC(VM& vm)
: mVM(vm) {
    mRegions[0].base = (uint8_t*)mmap(nullptr, HEAP_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    assert(mRegions[0].base);
    mRegions[1].base = (uint8_t*)mmap(nullptr, HEAP_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    assert(mRegions[1].base);
}
GC::~GC() {
    if(munmap(mRegions[0].base, HEAP_SIZE)) {
        perror("munmap()");
    }
    if(munmap(mRegions[1].base, HEAP_SIZE)) {
        perror("munmap()");
    }
}
uint8_t* GC::alloc(int32_t size) {
    auto ptr = mRegions[mActiveRegion].top();
    mRegions[mActiveRegion].offset += size;
    return ptr;
}
void GC::markAndSweep() {
    //Stopwatch stopwatch{"GC"};
    //printf("Before size: %i\n", (int)mRegions[mActiveRegion].offset);
    mRegions[!mActiveRegion].offset = 0;
    mMovedPointers.clear();
    mVM.generateStacktrace([this](const uint8_t* ptr, const Datatype& type, const std::string& name) {
        searchForPtrs((uint8_t*)ptr, type, ScanningHeapOrStack::Stack);
    }, {});
    mActiveRegion = !mActiveRegion;
    //printf("After size: %i\n", (int)mRegions[mActiveRegion].offset);
}
bool GC::copyToOther(uint8_t** ptr, size_t len) {
    auto maybePointerAddress = mMovedPointers.find(*ptr);
    if(maybePointerAddress == mMovedPointers.end()) {
        uint8_t* newPtr = mRegions[!mActiveRegion].top();
        memcpy(newPtr, *ptr, len);
        mRegions[!mActiveRegion].offset += len;
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
    case DatatypeCategory::f32:
    case DatatypeCategory::f64:
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
    default:
        assert(false);
    }
}
void GC::requestCollection() {
    callsSinceLastRun++;
    if(callsSinceLastRun > 400000) {
        markAndSweep();
        callsSinceLastRun = 0;
    }
}
}