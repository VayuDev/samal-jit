#pragma once
#include "Forward.hpp"

namespace samal {

struct EnumField {
    std::string name;
    std::vector<Datatype> params;
    inline bool operator==(const EnumField& other) const {
        return name == other.name && params == other.params;
    }
    inline bool operator!=(const EnumField& other) const {
        return !operator==(other);
    }
};

}