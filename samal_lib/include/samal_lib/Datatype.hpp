#pragma once
#include "AstHelpers.hpp"
#include "Forward.hpp"
#include <map>
#include <samal_lib/Util.hpp>
#include <stdexcept>
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
    struct EnumInfo;
public:
    Datatype();
    static Datatype createEmptyTuple();
    static Datatype createListType(Datatype baseType);
    static Datatype createStructType(const std::string& name, const std::vector<Parameter>& params, std::vector<std::string> templateParams);
    static Datatype createEnumType(const std::string& name, const std::vector<EnumField>& params, std::vector<std::string> templateParams);
    static Datatype createSimple(DatatypeCategory category);
    static Datatype createFunctionType(Datatype returnType, std::vector<Datatype> params);
    static Datatype createTupleType(std::vector<Datatype> params);
    static Datatype createUndeterminedIdentifierType(std::string name);
    static Datatype createUndeterminedIdentifierType(const IdentifierNode& name);

    Datatype(const Datatype& other);
    Datatype& operator=(const Datatype& other);

    [[nodiscard]] std::string toString() const;

    [[nodiscard]] const std::pair<Datatype, std::vector<Datatype>>& getFunctionTypeInfo() const&;
    [[nodiscard]] const std::vector<Datatype>& getTupleInfo() const;
    [[nodiscard]] inline const Datatype& getListContainedType() const {
        if(mCategory != DatatypeCategory::list) {
            throw std::runtime_error{ "This is not a list!" };
        }
        return std::get<Datatype>(*mFurtherInfo);
    }
    [[nodiscard]] const std::string& getUndeterminedIdentifierString() const;
    [[nodiscard]] const std::vector<Datatype>& getUndeterminedIdentifierTemplateParams() const;
    [[nodiscard]] const StructInfo& getStructInfo() const;
    [[nodiscard]] const EnumInfo& getEnumInfo() const;

    bool operator==(const Datatype& other) const;
    bool operator!=(const Datatype& other) const;
    [[nodiscard]] inline DatatypeCategory getCategory() const {
        return mCategory;
    }

    [[nodiscard]] bool isInteger() const;
    [[nodiscard]] size_t getSizeOnStack() const;

    enum class AllowIncompleteTypes {
        Yes,
        No
    };
    [[nodiscard]] Datatype completeWithTemplateParameters(const std::map<std::string, Datatype>& templateParams, const std::vector<std::string>& modules, AllowIncompleteTypes = AllowIncompleteTypes::No) const;
    [[nodiscard]] Datatype completeWithSavedTemplateParameters(AllowIncompleteTypes = AllowIncompleteTypes::No) const;

private:
    enum class InternalCall {
        Yes,
        No
    };
    [[nodiscard]] Datatype completeWithTemplateParameters(const std::map<std::string, Datatype>& templateParams, const std::vector<std::string>& modules, InternalCall internalCall, AllowIncompleteTypes) const;
    struct StructInfo {
        std::string name;
        struct StructElement;
        std::vector<StructElement> fields;
        std::vector<std::string> templateParams;
        inline bool operator==(const StructInfo& other) const {
            return name == other.name && fields == other.fields && templateParams == other.templateParams;
        }
        inline bool operator!=(const StructInfo& other) const {
            return !operator==(other);
        }
    };
    struct EnumInfo {
        std::string name;
        std::vector<EnumField> fields;
        std::vector<std::string> templateParams;
        [[nodiscard]] int32_t getLargestFieldSize() const;
        [[nodiscard]] int32_t getIndexSize() const;
        [[nodiscard]] int32_t getLargestFieldSizePlusIndex() const;
        inline bool operator==(const EnumInfo& other) const {
            return name == other.name && fields == other.fields && templateParams == other.templateParams;
        }
        inline bool operator!=(const EnumInfo& other) const {
            return !operator==(other);
        }
    };
    struct IdentifierInfo {
        std::string name;
        std::vector<Datatype> templateParams;
        inline bool operator==(const IdentifierInfo& other) const {
            return name == other.name && templateParams == other.templateParams;
        }
        inline bool operator!=(const IdentifierInfo& other) const {
            return !operator==(other);
        }
    };
    using ContainedFurtherInfoType = std::variant<
        std::monostate,
        IdentifierInfo,
        sp<class EnumDeclarationNode>,
        sp<class StructDeclarationNode>,
        std::pair<Datatype, std::vector<Datatype>>,
        std::vector<Datatype>,
        Datatype,
        StructInfo,
        EnumInfo>;
    up<ContainedFurtherInfoType> mFurtherInfo;

    DatatypeCategory mCategory;
    Datatype(DatatypeCategory, ContainedFurtherInfoType);

    // If a Datatype gets created by completeWithTemplateParameters(), the created type and all its children need
    // a pointer to the original undetermined identifier replacement map in case the type is recursive/infinite.
    // You can then call completeWithSavedTemplateParameters on those types to call completeWithTemplateParameters() with
    // the stored map, going down one recursion level.
    // This system is necessary for infinite types that contain themselves as it wouldn't be possible
    // to complete them in one go until there is nothing left.
    struct UndeterminedIdentifierCompletionInfo {
        std::map<std::string, Datatype> map;
        std::vector<std::string> includedModules;
    };
    void attachUndeterminedIdentifierMap(sp<UndeterminedIdentifierCompletionInfo> map);
    sp<UndeterminedIdentifierCompletionInfo> mUndefinedTypeReplacementMap;
};
struct Datatype::StructInfo::StructElement {
    std::string name;
    Datatype type;
    inline bool operator==(const StructElement& other) const {
        return name == other.name && type == other.type;
    }
    inline bool operator!=(const StructElement& other) const {
        return !operator==(other);
    }
};

static inline size_t getSimpleSize(DatatypeCategory type) {
    return Datatype::createSimple(type).getSizeOnStack();
}

}