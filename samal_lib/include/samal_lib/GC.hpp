#pragma once
#include "Forward.hpp"
#include "Util.hpp"
#include <cstdint>
#include <vector>

namespace samal {

class GC final {
public:
    explicit GC(VM&);
    GC(const GC&) = delete;
    void operator=(const GC&) = delete;
    uint8_t* alloc(int32_t num);
    void requestCollection();

private:
    int32_t callsSinceLastRun { 0 };
    struct Allocation {
        bool found{ false };
        std::unique_ptr<uint8_t, void (*)(uint8_t*)> ptr;
    };
    VM& mVM;
    std::vector<Allocation> mAllocations;

    void searchForPtrs(const uint8_t* ptr, const Datatype& type);
    void markPtrAsFound(const uint8_t* ptr);
    void markAndSweep();
};

}