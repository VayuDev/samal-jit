#include "samal_lib/ExternalVMValue.hpp"
#include "samal_lib/VM.hpp"
#include <cassert>
#include <sstream>

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
    case DatatypeCategory::function:
    case DatatypeCategory::i64: {
        auto val = std::get<int64_t>(mValue);
        return std::vector<uint8_t>{ (uint8_t)(val >> 0), (uint8_t)(val >> 8), (uint8_t)(val >> 16), (uint8_t)(val >> 24), (uint8_t)(val >> 32), (uint8_t)(val >> 40), (uint8_t)(val >> 48), (uint8_t)(val >> 56) };
    }
    case DatatypeCategory::tuple: {
        std::vector<uint8_t> bytes;
        for(auto& child : std::get<std::vector<ExternalVMValue>>(mValue)) {
            auto childBytes = child.toStackValue(vm);
            bytes.insert(bytes.end(), childBytes.begin(), childBytes.end());
        }
        return bytes;
    }
    default:
        assert(false);
    }
}
std::string ExternalVMValue::dump() const {
    std::string ret{ "{type: " };
    ret += mType.toString();
    ret += ", value: ";
    switch(mType.getCategory()) {
    case DatatypeCategory::i32:
        ret += std::to_string(std::get<int32_t>(mValue));
        break;
    case DatatypeCategory::i64:
        ret += std::to_string(std::get<int64_t>(mValue));
        break;
    case DatatypeCategory::tuple:
        ret += "(";
        for(auto& child : std::get<std::vector<ExternalVMValue>>(mValue)) {
            ret += child.dump();
            ret += ", ";
        }
        ret += ")";
        break;
    case DatatypeCategory::function: {
        std::stringstream ss;
        ss << std::hex << std::get<int64_t>(mValue);
        ret += ss.str();
        break;
    }
    default:
        ret += "<unknown>";
    }
    ret += "}";
    return ret;
}
ExternalVMValue ExternalVMValue::wrapStackedValue(Datatype type, VM& vm, size_t stackOffset) {
    auto stackEnd = vm.getStack().getBasePtr() + vm.getStack().getSize();
    switch(type.getCategory()) {
    case DatatypeCategory::i32:
        return ExternalVMValue{ type, *(int32_t*)(stackEnd - stackOffset - type.getSizeOnStack()) };
    case DatatypeCategory::function:
    case DatatypeCategory::i64:
        return ExternalVMValue{ type, *(int64_t*)(stackEnd - stackOffset - type.getSizeOnStack()) };
    case DatatypeCategory::tuple: {
        std::vector<ExternalVMValue> children;
        auto reversedTypes = type.getTupleInfo();
        std::reverse(reversedTypes.begin(), reversedTypes.end());

        size_t totalSizeOnStack = stackOffset;
        children.reserve(reversedTypes.size());
        for(auto& childType : reversedTypes) {
            children.emplace_back(ExternalVMValue::wrapStackedValue(childType, vm, totalSizeOnStack));
            totalSizeOnStack += childType.getSizeOnStack();
        }
        std::reverse(children.begin(), children.end());
        return ExternalVMValue{ type, std::move(children) };
    }
    default:
        return ExternalVMValue{ type, std::monostate{} };
    }
}
ExternalVMValue ExternalVMValue::wrapEmptyTuple() {
    return ExternalVMValue(Datatype::createEmptyTuple(), std::vector<ExternalVMValue>{});
}
}