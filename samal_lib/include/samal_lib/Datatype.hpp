#pragma once
#include "Forward.hpp"
#include <map>
#include <samal_lib/Util.hpp>
#include <variant>
#include <vector>

namespace samal {

enum class DatatypeCategory {
    invalid,
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
    struct StructInfo;
public:
    Datatype();
    explicit Datatype(DatatypeCategory category);
    Datatype(Datatype returnType, std::vector<Datatype> params);
    explicit Datatype(std::vector<Datatype> params);
    explicit Datatype(std::string identifierName);
    [[nodiscard]] std::string toString() const;

    [[nodiscard]] const std::pair<sp<Datatype>, std::vector<Datatype>>& getFunctionTypeInfo() const&;
    [[nodiscard]] const std::vector<Datatype>& getTupleInfo() const;
    [[nodiscard]] const Datatype& getListInfo() const;
    [[nodiscard]] const std::string& getUndeterminedIdentifierInfo() const;
    [[nodiscard]] const StructInfo& getStructInfo() const;

    bool operator==(const Datatype& other) const;
    bool operator!=(const Datatype& other) const;
    [[nodiscard]] DatatypeCategory getCategory() const;

    [[nodiscard]] bool isInteger() const;
    [[nodiscard]] size_t getSizeOnStack() const;

    static Datatype createEmptyTuple();
    static Datatype createListType(Datatype baseType);
    static Datatype createStructType(const std::string& name, const std::vector<Parameter>& params, const std::vector<Datatype>& templateParams);

    [[nodiscard]] Datatype completeWithTemplateParameters(const std::map<std::string, Datatype>& templateParams) const;

    [[nodiscard]] bool hasUndeterminedTemplateTypes() const;

private:
    struct StructInfo {
        std::string name;
        struct StructElement;
        std::vector<StructElement> elements;
        std::vector<std::string> templateParams;
        bool operator==(const StructInfo&) const = default;
    };
    std::variant<
        std::monostate,
        std::string,
        sp<class EnumDeclarationNode>,
        sp<class StructDeclarationNode>,
        std::pair<sp<Datatype>, std::vector<Datatype>>,
        std::vector<Datatype>,
        sp<Datatype>,
        StructInfo>
        mFurtherInfo;
    DatatypeCategory mCategory;
    Datatype(DatatypeCategory, decltype(mFurtherInfo));
};
struct Datatype::StructInfo::StructElement {
    std::string name;
    Datatype baseType;
    std::function<Datatype()> lazyType;
    inline bool operator==(const StructElement& other) const {
        return name == other.name && baseType == other.baseType;
    }
};

static inline size_t getSimpleSize(DatatypeCategory type) {
    return Datatype{ type }.getSizeOnStack();
}

}