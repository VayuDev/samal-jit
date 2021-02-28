#include "samal_lib/GC.hpp"
#include "samal_lib/Program.hpp"
#include "samal_lib/VM.hpp"
#include <algorithm>

namespace samal {

GC::GC(VM& vm)
: mVM(vm) {
}
GC::~GC() {
    for(auto& alloc : mAllocations) {
        free(alloc);
    }
    mAllocations.clear();
}
uint8_t* GC::alloc(int32_t num) {
    auto ptr = static_cast<uint8_t*>(malloc(num));
    try {
        mAllocations.emplace(ptr);
        return ptr;
    } catch(...) {
        free(ptr);
        return nullptr;
    }
}
void GC::markAndSweep() {
    //printf("Before: %lu\n", mAllocations.size());
    //Stopwatch stopwatch{"GC"};
    // mark all as not found
    //Stopwatch firstStopwatch{"GC first"};
    mFoundAllocations.clear();
    //firstStopwatch.stop();

    // start marking the ones we find
    //Stopwatch secondStopwatch{"GC second"};
    mVM.generateStacktrace([this](const uint8_t* ptr, const Datatype& type, const std::string& name) {
        searchForPtrs(ptr, type);
    },
        {});
    //secondStopwatch.stop();

    // Stopwatch thirdStopwatch{"GC third"};
    for(auto it = mAllocations.begin(); it != mAllocations.end();) {
        if(!mFoundAllocations.contains(*it)) {
            free(*it);
            it = mAllocations.erase(it);
        } else {
            ++it;
        }
    }
    //thirdStopwatch.stop();
    //printf("After: %lu\n", mAllocations.size());
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
        const uint8_t* current = *(const uint8_t**)ptr;
        while(current) {
            bool alreadyFound = markPtrAsFound(current);
            if(alreadyFound) {
                break;
            }
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
        bool alreadyFound = markPtrAsFound(lambdaPtr);
        if(alreadyFound) {
            break;
        }
        int32_t capturedLambaTypesId;
        memcpy(&capturedLambaTypesId, lambdaPtr + 8, 4);
        searchForPtrs(lambdaPtr + 16, mVM.getProgram().auxiliaryDatatypes.at(capturedLambaTypesId));
        break;
    }
    case DatatypeCategory::struct_: {
        uint8_t* structPtr = *(uint8_t**)ptr;
        bool alreadyFound = markPtrAsFound(structPtr);
        if(alreadyFound) {
            break;
        }
        int32_t offset = 0;
        for(auto& field : type.getStructInfo().fields) {
            searchForPtrs(structPtr + offset, field.type);
            offset += field.type.getSizeOnStack();
        }
        break;
    }
    default:
        assert(false);
    }
}

bool GC::markPtrAsFound(const uint8_t* ptr) {
    return !mFoundAllocations.emplace((uint8_t*)ptr).second;
}
void GC::requestCollection() {
    callsSinceLastRun++;
    if(callsSinceLastRun > 20000) {
        markAndSweep();
        callsSinceLastRun = 0;
    }
}
}