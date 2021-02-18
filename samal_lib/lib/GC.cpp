#include "samal_lib/GC.hpp"
#include "samal_lib/Program.hpp"
#include "samal_lib/VM.hpp"

namespace samal {

GC::GC(VM& vm)
: mVM(vm) {
}
uint8_t* GC::alloc(int32_t num) {
    auto ptr = static_cast<uint8_t*>(malloc(num));
    mAllocations.emplace_back(
        Allocation{
            .found = false,
            .ptr = {
                ptr,
                [](uint8_t* ptr) {
                    free(ptr);
                } } });
    return ptr;
}
void GC::markAndSweep() {
    // mark all as not found
    for(auto& alloc: mAllocations) {
        alloc.found = false;
    }

    // start marking the ones we find
    auto stackTrace = mVM.generateStacktrace();
    for(auto& frame: stackTrace.stackFrames) {
        for(auto& var: frame.variables) {
            searchForPtrs(var.ptr, var.type);
        }
    }
    // sweep those we didn't find
    for(ssize_t i = mAllocations.size() - 1; i >= 0; --i) {
        auto& alloc = mAllocations.at(i);
        if(!alloc.found) {
#ifdef _DEBUG
            printf("Erasing %p\n", alloc.ptr.get());
#endif
            mAllocations.erase(mAllocations.begin() + i);
        } else {
#ifdef _DEBUG
            printf("Keeping %p\n", alloc.ptr.get());
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
            searchForPtrs(current + 8, type.getListInfo());
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
    auto maybeAllocation = std::find_if(mAllocations.begin(), mAllocations.end(), [ptr](const Allocation& alloc) {
        return alloc.ptr.get() == ptr;
    });
    if(maybeAllocation == mAllocations.end()) {
        throw std::runtime_error{"Allocation not found!"};
        assert(false);
    }
    assert(maybeAllocation != mAllocations.end());
    maybeAllocation->found = true;
}
void GC::requestCollection() {
    callsSinceLastRun++;
    if(callsSinceLastRun > 100000) {
        markAndSweep();
        callsSinceLastRun = 0;
    }
}
}