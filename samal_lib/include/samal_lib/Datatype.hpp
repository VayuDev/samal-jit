#pragma once
#include <samal_lib/Util.hpp>
#include <variant>
#include <vector>

namespace samal {

enum class DatatypeCategory {
    i64,
    i32,
    f64,
    f32,
    char_,
    bool_,
    undetermined_identifier,
    string,
    struct_,
    enum_,
    tuple,
    function,
    list,
};

class Datatype {
public:
    explicit Datatype(DatatypeCategory category);
    Datatype(Datatype returnType, std::vector<Datatype> params);
    explicit Datatype(std::vector<Datatype> params);
    explicit Datatype(std::string identifierName);
    [[nodiscard]] std::string toString() const;
    [[nodiscard]] const std::pair<sp<Datatype>, std::vector<Datatype>>& getFunctionTypeInfo() const&;
    [[nodiscard]] const std::vector<Datatype>& getTupleInfo() const;
    [[nodiscard]] const Datatype& getListInfo() const;
    bool operator==(const Datatype& other) const;
    bool operator!=(const Datatype& other) const;
    [[nodiscard]] DatatypeCategory getCategory() const;

    [[nodiscard]] bool isInteger() const;
    [[nodiscard]] size_t getSizeOnStack() const;

    static Datatype createEmptyTuple();
    static Datatype createListType(Datatype baseType);

private:
    std::variant<
        std::monostate,
        std::string,
        sp<class EnumDeclarationNode>,
        sp<class StructDeclarationNode>,
        std::pair<sp<Datatype>, std::vector<Datatype>>,
        std::vector<Datatype>,
        sp<Datatype>>
        mFurtherInfo;
    DatatypeCategory mCategory;
};

static inline size_t getSimpleSize(DatatypeCategory type) {
    return Datatype{ type }.getSizeOnStack();
}

}