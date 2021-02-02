#pragma once
#include "Datatype.hpp"
#include "Forward.hpp"

namespace samal {

class ExternalVMValue final {
public:
    ~ExternalVMValue();
    static ExternalVMValue wrapInt32(VM& vm, int32_t);
    static ExternalVMValue wrapInt64(VM& vm, int64_t);
    static ExternalVMValue wrapEmptyTuple(VM& vm);
    static ExternalVMValue wrapStackedValue(Datatype type, VM& vm, size_t stackOffset);
    static ExternalVMValue wrapFromPtr(Datatype type, VM& vm, const uint8_t* ptr);
    [[nodiscard]] const Datatype& getDatatype() const&;
    std::vector<uint8_t> toStackValue(VM& vm) const;
    [[nodiscard]] std::string dump() const;

    template<typename T>
    const auto& as() const {
        return std::get<T>(mValue);
    }

private:
    VM* mVM = nullptr;
    Datatype mType;
    std::variant<std::monostate, int32_t, int64_t, std::vector<ExternalVMValue>, const uint8_t*> mValue;

    explicit ExternalVMValue(VM& vm, Datatype type, decltype(mValue) val);
};

}