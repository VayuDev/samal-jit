#pragma once
#include "Forward.hpp"
#include "Util.hpp"
#include <cstdint>
#include <set>
#include <unordered_map>
#include <vector>
#include <array>
#include <sys/mman.h>

namespace samal {

class GC final {
public:
    explicit GC(VM&, const VMParameters& params);
    GC(const GC&) = delete;
    GC(GC&&) = delete;
    GC& operator=(const GC&) = delete;
    GC& operator=(GC&&) = delete;
    uint8_t* alloc(int32_t num);
    void requestCollection();

private:
    int32_t mFunctionCallsSinceLastRun{ 0 };
    int32_t mConfigFunctionsCallsPerGCRun{ 0 };
    VM& mVM;
    struct Region final {
        uint8_t *base{ nullptr };
        size_t offset = 0;
        size_t size = 0;

        Region() = default;
        explicit Region(size_t len);
        ~Region();
        // move constructors
        Region(Region&&) noexcept;
        Region& operator=(Region&&) noexcept ;
        // delete copy constructor
        Region(const Region&) = delete;
        Region& operator=(const Region&) = delete;

        inline uint8_t* top() {
            return base + offset;
        }
    };

    Region& getActiveRegion();
    Region& getOtherRegion();

    struct ArrayDeleter {
        void operator()(uint8_t* toDelete) {
            free(toDelete);
        }
    };
    struct TemporaryAllocation {
        std::unique_ptr<uint8_t, ArrayDeleter> data;
        size_t len;
    };
    // This array contains allocations that had to be performed using new because the active
    // region was full. As we can't garbage collect and resize a region at any point in time, we
    // need to instead store it in this temporary array and copy it over to a new, larger page
    // once we hit the next garbage collection.
    std::vector<TemporaryAllocation> mTemporaryAllocations;

    std::array<Region, 2> mRegions{Region{}, Region{}};
    size_t mActiveRegion{ 0 };
    [[nodiscard]] bool isInOtherRegion(uint8_t* ptr);

    enum class ScanningHeapOrStack {
        Heap,
        Stack
    };
    void searchForPtrs(uint8_t* ptr, const Datatype& type, ScanningHeapOrStack);
    uint8_t* copyToOther(uint8_t** ptr, size_t len);
    uint8_t* alloc(int32_t len, size_t heapIndex);
    void performGarbageCollection();
};

}
