#pragma once
#include "Forward.hpp"
#include "Util.hpp"
#include <cstdint>
#include <vector>

namespace samal {

class GC final {
public:
    GC() = default;
    GC(const GC&) = delete;
    void operator=(const GC&) = delete;
    uint8_t* alloc(int32_t num);
    void markAndSweep(int32_t ip, const Stack& stack, const Program& program);

private:
    struct Allocation {
        bool found{ false };
        std::unique_ptr<uint8_t, void (*)(uint8_t*)> ptr;
    };
    std::vector<Allocation> mAllocations;
};

}