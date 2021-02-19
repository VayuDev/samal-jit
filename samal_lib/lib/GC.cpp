#include "samal_lib/GC.hpp"
#include "samal_lib/Program.hpp"
#include "samal_lib/VM.hpp"

namespace samal {

GC::GC(VM& vm)
: mVM(vm) {
}
GC::~GC() {
    for(auto& alloc: mAllocations) {
        free(alloc.first);
    }
    mAllocations.clear();
}
uint8_t* GC::alloc(int32_t num) {
    auto ptr = static_cast<uint8_t*>(malloc(num));
    try {
        mAllocations[ptr] = false;
        return ptr;
    } catch(...) {
        free(ptr);
        return nullptr;
    }
}
void GC::markAndSweep() {
    // mark all as not found
    for(auto& alloc: mAllocations) {
        alloc.second = false;
    }

    // start marking the ones we find
    mVM.generateStacktrace([this](const uint8_t* ptr, const Datatype& type, const std::string& name){
        searchForPtrs(ptr, type);
    }, {});
    // sweep those we didn't find
    for(auto it = mAllocations.begin(); it != mAllocations.end();) {
        auto& alloc = *it;
        if(!alloc.second) {
#ifdef _DEBUG
            printf("Erasing %p\n", alloc.first);
#endif
            free(alloc.first);
            it = mAllocations.erase(it);
        } else {
            ++it;
#ifdef _DEBUG
            printf("Keeping %p\n", alloc.first);
#endif
        }
    }
}
void GC::searchForPtrs(const uint8_t* ptr, const Datatype& type) {
    switch(type.getCategory()) {
    case DatatypeCategory::bool_:
    case DatatypeCategory::i32:
    case DatatypeCategory::i64:
    case DatatypeCategory::f32:
    case DatatypeCategory::f64:
        break;
    case DatatypeCategory::tuple: {
        int32_t offset = 0;
        for(auto& element : type.getTupleInfo()) {
            searchForPtrs(ptr + offset, element);
            offset += element.getSizeOnStack();
        }
        break;
    }
    case DatatypeCategory::list: {
        const uint8_t *current = *(const uint8_t**)ptr;
        while(current) {
            markPtrAsFound(current);
            searchForPtrs(current + 8, type.getListContainedType());
            current = *(const uint8_t**)current;
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
        assert((uint64_t)lambdaPtr % 2 == 0);
        markPtrAsFound(lambdaPtr);
        int32_t capturedLambaTypesId;
        memcpy(&capturedLambaTypesId, lambdaPtr + 8, 4);
        searchForPtrs(lambdaPtr + 16, mVM.getProgram().auxiliaryDatatypes.at(capturedLambaTypesId));
        break;
    }
    case DatatypeCategory::struct_: {
        uint8_t* structPtr = *(uint8_t**)ptr;
        markPtrAsFound(structPtr);
        int32_t offset = 0;
        for(auto& field: type.getStructInfo().elements) {
            searchForPtrs(structPtr + offset, field.baseType);
            offset += field.baseType.getSizeOnStack();
        }
        break;
    }
    default:
        assert(false);
    }
}
void GC::markPtrAsFound(const uint8_t* ptr) {
    auto maybeAllocation = mAllocations.find((uint8_t*)ptr);
    if(maybeAllocation == mAllocations.end()) {
        assert(false);
        throw std::runtime_error{"Allocation not found!"};
    }
    maybeAllocation->second = true;
}
void GC::requestCollection() {
    callsSinceLastRun++;
    if(callsSinceLastRun > 2000000) {
        printf("Running GC\n");
        markAndSweep();
        callsSinceLastRun = 0;
    }
}
}