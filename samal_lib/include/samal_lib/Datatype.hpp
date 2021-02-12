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
    static Datatype createEmptyTuple();
    static Datatype createListType(Datatype baseType);
    static Datatype createStructType(const std::string& name, const std::vector<Parameter>& params, const std::vector<Datatype>& templateParams);
    static Datatype createSimple(DatatypeCategory category);
    static Datatype createFunctionType(Datatype returnType, std::vector<Datatype> params);
    static Datatype createTupleType(std::vector<Datatype> params);
    static Datatype createUndeterminedIdentifierType(std::string name);

    Datatype(const Datatype& other);
    Datatype& operator=(const Datatype& other);

    [[nodiscard]] std::string toString() const;

    [[nodiscard]] const std::pair<Datatype, std::vector<Datatype>>& getFunctionTypeInfo() const&;
    [[nodiscard]] const std::vector<Datatype>& getTupleInfo() const;
    [[nodiscard]] const Datatype& getListInfo() const;
    [[nodiscard]] const std::string& getUndeterminedIdentifierString() const;
    [[nodiscard]] const StructInfo& getStructInfo() const;

    bool operator==(const Datatype& other) const;
    bool operator!=(const Datatype& other) const;
    [[nodiscard]] DatatypeCategory getCategory() const;

    [[nodiscard]] bool isInteger() const;
    [[nodiscard]] size_t getSizeOnStack() const;

    [[nodiscard]] Datatype completeWithTemplateParameters(const std::map<std::string, Datatype>& templateParams) const;
    [[nodiscard]] Datatype completeWithSavedTemplateParameters() const;

    [[nodiscard]] bool hasUndeterminedTemplateTypes() const;

private:
    struct StructInfo {
        std::string name;
        struct StructElement;
        std::vector<StructElement> elements;
        std::vector<std::string> templateParams;
        bool operator==(const StructInfo&) const = default;
    };
    using ContainedFurtherInfoType = std::variant<
        std::monostate,
        std::string,
        sp<class EnumDeclarationNode>,
        sp<class StructDeclarationNode>,
        std::pair<Datatype, std::vector<Datatype>>,
        std::vector<Datatype>,
        Datatype,
        StructInfo>;
    up<ContainedFurtherInfoType> mFurtherInfo;

    DatatypeCategory mCategory;
    Datatype(DatatypeCategory, ContainedFurtherInfoType);

    // If a Datatype gets created by completeWithTemplateParameters(), the created type and all its children need
    // a pointer to the original undetermined identifier replacement map in case the type is recursive/infinite.
    // You can then call completeWithSavedTemplateParameters on those types to call completeWithTemplateParameters() with
    // the stored map, going down one recursion level.
    // This system is necessary for infinite types that contain themselves as it wouldn't be possible
    // to complete them in one go until there is nothing left.
    void attachUndeterminedIdentifierMap(sp<std::map<std::string, Datatype>> map);
    sp<std::map<std::string, Datatype>> mUndefinedTypeReplacementMap;
};
struct Datatype::StructInfo::StructElement {
    std::string name;
    Datatype baseType;
    bool operator==(const StructElement&) const = default;
};

static inline size_t getSimpleSize(DatatypeCategory type) {
    return Datatype::createSimple(type).getSizeOnStack();
}

}