#include "samal_lib/ExternalVMValue.hpp"
#include "samal_lib/VM.hpp"
#include <cassert>

namespace samal {

ExternalVMValue::~ExternalVMValue() = default;

ExternalVMValue::ExternalVMValue(Datatype type, decltype(mValue) val)
: mType(std::move(type)), mValue(std::move(val)) {
}
ExternalVMValue ExternalVMValue::wrapInt32(int32_t val) {
    return ExternalVMValue(Datatype{ DatatypeCategory::i32 }, val);
}
ExternalVMValue ExternalVMValue::wrapInt64(int64_t val) {
    return ExternalVMValue(samal::Datatype(DatatypeCategory::i64), val);
}
const Datatype& ExternalVMValue::getDatatype() const& {
    return mType;
}
std::vector<uint8_t> ExternalVMValue::toStackValue(VM& vm) const {
    switch(mType.getCategory()) {
    case DatatypeCategory::i32: {
        auto val = std::get<int32_t>(mValue);
#ifdef x86_64_BIT_MODE
        return std::vector<uint8_t>{ (uint8_t)(val >> 0), (uint8_t)(val >> 8), (uint8_t)(val >> 16), (uint8_t)(val >> 24), 0, 0, 0, 0 };
#else
        return std::vector<uint8_t>{ (uint8_t)(val >> 0), (uint8_t)(val >> 8), (uint8_t)(val >> 16), (uint8_t)(val >> 24) };
#endif
    }
    case DatatypeCategory::i64: {
        auto val = std::get<int64_t>(mValue);
        return std::vector<uint8_t>{ (uint8_t)(val >> 0), (uint8_t)(val >> 8), (uint8_t)(val >> 16), (uint8_t)(val >> 24), (uint8_t)(val >> 32), (uint8_t)(val >> 40), (uint8_t)(val >> 48), (uint8_t)(val >> 56) };
    }
    default:
        assert(false);
    }
}
}