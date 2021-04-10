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
    static ExternalVMValue wrapChar(VM& vm, int32_t charUTF32Value);
    static ExternalVMValue wrapString(VM& vm, const std::string& string);
    static ExternalVMValue wrapStringAsByteArray(VM& vm, const std::string& string);
    static ExternalVMValue wrapEnum(VM& vm, const Datatype& enumType, const std::string& fieldName, std::vector<ExternalVMValue>&& elements);
    static ExternalVMValue wrapStackedValue(Datatype type, VM& vm, size_t stackOffset);
    static ExternalVMValue wrapFromPtr(Datatype type, VM& vm, const uint8_t* ptr);
    [[nodiscard]] const Datatype& getDatatype() const&;
    std::vector<uint8_t> toStackValue(VM& vm) const;
    [[nodiscard]] std::string dump() const;

    [[nodiscard]] bool isWrappingString() const;
    [[nodiscard]] std::string toCPPString() const;
    [[nodiscard]] std::vector<uint8_t> toByteBuffer() const;

    template<typename T>
    const auto& as() const {
        return std::get<T>(mValue);
    }

private:
    VM* mVM = nullptr;
    Datatype mType;
    struct StructValue {
        std::string name;
        struct Field;
        std::vector<Field> fields;
    };
    struct EnumValue {
        int32_t selectedFieldIndex;
        std::vector<ExternalVMValue> elements;
    };
    std::variant<std::monostate, int32_t, int64_t, std::vector<ExternalVMValue>, StructValue, EnumValue, const uint8_t*, uint8_t, bool> mValue;

    explicit ExternalVMValue(VM& vm, Datatype type, decltype(mValue) val);
};

struct ExternalVMValue::StructValue::Field {
    ExternalVMValue value;
    std::string name;
};

}