#pragma once
#include "Forward.hpp"
#include "Util.hpp"
#include <cstdint>
#include <set>
#include <unordered_set>
#include <vector>
#include <sys/mman.h>

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
    int32_t callsSinceLastRun{ 0 };
    VM& mVM;
    struct Region {
        uint8_t *base{ nullptr };
        size_t offset = 0;
        inline uint8_t* top() {
            return base + offset;
        }
    };
    std::array<Region, 2> mRegions{ Region{}, Region{} };
    size_t mActiveRegion{ 0 };
    std::unordered_map<uint8_t*, uint8_t*> mMovedPointers;

    enum class ScanningHeapOrStack {
        Heap,
        Stack
    };
    void searchForPtrs(uint8_t* ptr, const Datatype& type, ScanningHeapOrStack);
    bool copyToOther(uint8_t** ptr, size_t len);
    void markAndSweep();
};

}