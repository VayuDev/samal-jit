#include "samal_lib/ExternalVMValue.hpp"
#include "samal_lib/VM.hpp"
#include <cassert>
#include <sstream>

namespace samal {

ExternalVMValue::~ExternalVMValue() = default;

ExternalVMValue::ExternalVMValue(VM& vm, Datatype type, decltype(mValue) val)
: mVM(&vm), mType(std::move(type)), mValue(std::move(val)) {
}
ExternalVMValue ExternalVMValue::wrapInt32(VM& vm, int32_t val) {
    return ExternalVMValue(vm, Datatype::createSimple(DatatypeCategory::i32), val);
}
ExternalVMValue ExternalVMValue::wrapInt64(VM& vm, int64_t val) {
    return ExternalVMValue(vm, Datatype::createSimple(DatatypeCategory::i64), val);
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
    std::string ret;
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
            if(&child != &std::get<std::vector<ExternalVMValue>>(mValue).back()) {
                ret += ", ";
            }
        }
        ret += ")";
        break;
    case DatatypeCategory::function: {
        std::stringstream ss;
        ss << std::hex << std::get<int64_t>(mValue);
        ret += ss.str();
        break;
    }
    case DatatypeCategory::list: {
        ret += "[";
        auto* current = std::get<const uint8_t*>(mValue);
        while(current != nullptr) {
            ret += ExternalVMValue::wrapFromPtr(mType.getListInfo(), *mVM, current + 8).dump();
            current = *(uint8_t**)current;
            if(current != nullptr)
                ret += ", ";
        }
        ret += "]";
        break;
    }
    case DatatypeCategory::struct_: {
        ret += mType.getStructInfo().name + "{";
        auto ptr = std::get<const uint8_t*>(mValue);
        for(auto& element: mType.getStructInfo().elements) {
            auto elementValue = ExternalVMValue::wrapFromPtr(element.baseType, *mVM, ptr);
            // Sidenote: elementValue can actually have a different type (getDataype()) than we pass in with element.baseType.
            //           This happens if the element.baseType is incomplete (undetermined identifier), which will lead to wrapFromPtr calling itself
            //           recursively with the completed type. See Datatype.hpp for more information.
            ret += element.name;
            ret += ": ";
            ret += elementValue.dump();
            if(&element != &mType.getStructInfo().elements.back()) {
                ret += ", ";
                ptr += elementValue.getDatatype().getSizeOnStack();
            }
        }
        ret += "}";
        break;
    }
    default:
        ret += "<unknown>";
    }
    return ret;
}
ExternalVMValue ExternalVMValue::wrapStackedValue(Datatype type, VM& vm, size_t stackOffset) {
    auto stackTop = vm.getStack().getTopPtr();
    return wrapFromPtr(type, vm, stackTop + stackOffset);
}
ExternalVMValue ExternalVMValue::wrapEmptyTuple(VM& vm) {
    return ExternalVMValue(vm, Datatype::createEmptyTuple(), std::vector<ExternalVMValue>{});
}
ExternalVMValue ExternalVMValue::wrapFromPtr(Datatype type, VM& vm, const uint8_t* ptr) {
    switch(type.getCategory()) {
    case DatatypeCategory::i32:
        return ExternalVMValue{ vm, type, *(int32_t*)(ptr) };
    case DatatypeCategory::function:
    case DatatypeCategory::i64:
        return ExternalVMValue{ vm, type, *(int64_t*)(ptr) };
    case DatatypeCategory::tuple: {
        std::vector<ExternalVMValue> children;
        auto reversedTypes = type.getTupleInfo();
        std::reverse(reversedTypes.begin(), reversedTypes.end());

        children.reserve(reversedTypes.size());
        for(auto& childType : reversedTypes) {
            children.emplace_back(ExternalVMValue::wrapFromPtr(childType, vm, ptr));
            ptr += childType.getSizeOnStack();
        }
        std::reverse(children.begin(), children.end());
        return ExternalVMValue{ vm, type, std::move(children) };
    }
    case DatatypeCategory::struct_:
    case DatatypeCategory::list: {
        return ExternalVMValue{ vm, type, *(uint8_t**)(ptr) };
    }
    case DatatypeCategory::undetermined_identifier: {
        return wrapFromPtr(type.completeWithSavedTemplateParameters(), vm, ptr);
    }
    default:
        return ExternalVMValue{ vm, type, std::monostate{} };
    }
}
}