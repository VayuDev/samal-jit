#pragma once
#include "Datatype.hpp"
#include "Forward.hpp"

namespace samal {

class ExternalVMValue final {
public:
    ~ExternalVMValue();
    static ExternalVMValue wrapInt32(int32_t);
    static ExternalVMValue wrapInt64(int64_t);
    [[nodiscard]] const Datatype& getDatatype() const&;
    std::vector<uint8_t> toStackValue(VM& vm) const;

private:
    Datatype mType;
    std::variant<int32_t, int64_t> mValue;

    explicit ExternalVMValue(Datatype type, decltype(mValue) val);
};

}