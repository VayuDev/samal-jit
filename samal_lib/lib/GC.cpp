#include "samal_lib/GC.hpp"
#include "samal_lib/Program.hpp"

namespace samal {

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
void GC::markAndSweep(int32_t ip, const Stack& stack, const Program& program) {
}
}