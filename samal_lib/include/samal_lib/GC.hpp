#pragma once
#include "Forward.hpp"
#include "Util.hpp"
#include <cstdint>
#include <vector>
#include <list>

namespace samal {

class GC final {
public:
    explicit GC(VM&);
    ~GC();
    GC(const GC&) = delete;
    void operator=(const GC&) = delete;
    uint8_t* alloc(int32_t num);
    void requestCollection();

private:
    int32_t callsSinceLastRun { 0 };
    VM& mVM;
    std::unordered_map<uint8_t*, bool> mAllocations;

    void searchForPtrs(const uint8_t* ptr, const Datatype& type);
    bool markPtrAsFound(const uint8_t* ptr);
    void markAndSweep();
};

}