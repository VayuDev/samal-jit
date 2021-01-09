#pragma once
#include "peg_parser/PegForward.hpp"
#include "peg_parser/PegUtil.hpp"
#include <any>
#include <cassert>
#include <cstddef>
#include <functional>
#include <map>
#include <optional>
#include <vector>

namespace peg {

struct ParsingState {
    size_t tokenizerState = 0;
    inline bool operator==(const ParsingState& pOther) const {
        return tokenizerState == pOther.tokenizerState;
    }
};

class Any final {
public:
    Any() = default;
    template<typename T, std::enable_if_t<!std::is_same<T, Any>::value, bool> = true>
    inline Any(T&& ele) {
        data = new T { std::forward<T>(ele) };
        destructor = [](void* d) {
            delete (T*)d;
        };
    }
    inline ~Any() {
        if (destructor)
            destructor(data);
    }
    Any(const Any& other) = delete;
    Any& operator=(const Any& other) = delete;
    inline Any(Any&& other) {
        data = other.data;
        destructor = other.destructor;
        other.data = nullptr;
        other.destructor = {};
    }
    inline Any& operator=(Any&& other) {
        data = other.data;
        destructor = other.destructor;
        other.data = nullptr;
        other.destructor = {};
        return *this;
    }

    template<typename T>
    inline auto get() const {
        return static_cast<T>(data);
    }

    template<typename T>
    inline T move() {
        assert(data);
        auto cpy = data;
        data = nullptr;
        destructor = {};
        return static_cast<T>(cpy);
    }
    template<typename T>
    inline T moveValue() {
        static_assert(!std::is_pointer_v<T>, "If you want a pointer to the data, use move() instead");
        assert(data);
        T movedValue = std::move(*static_cast<T*>(data));
        delete (T*)data;
        data = nullptr;
        destructor = {};
        return movedValue;
    }

private:
    void* data = nullptr;
    std::function<void(void* d)> destructor;
};

struct MatchInfo {
    const char* start = nullptr;
    size_t len = 0;
    std::pair<size_t, size_t> sourcePosition;
    std::optional<size_t> choice;
    Any result;
    std::vector<MatchInfo> subs;

    [[nodiscard]] inline const char* startTrimmed() const {
        auto retStart = start;
        while ((*retStart == ' ' || *retStart == '\n' || *retStart == '\t') && retStart < (start + len)) {
            retStart += 1;
        }
        return retStart;
    }
    [[nodiscard]] inline const char* endTrimmed() const {
        auto retEnd = start + len;
        while ((*(retEnd - 1) == ' ' || *(retEnd - 1) == '\n' || *(retEnd - 1) == '\t') && start < retEnd) {
            retEnd -= 1;
        }
        return retEnd;
    }
    inline const MatchInfo& operator[](size_t index) const {
        return subs.at(index);
    };
    inline MatchInfo& operator[](size_t index) {
        return subs.at(index);
    };
};
using RuleCallback = std::function<Any(MatchInfo& info)>;

struct Rule {
    sp<ParsingExpression> expr;
    RuleCallback callback;

    Rule& operator<<(const char* ruleString);
    Rule& operator>>(RuleCallback&& ruleCallback);
};

using RuleMap = std::map<std::string, Rule>;

struct ParsingFailInfo {
    bool eof = false;
    up<class ExpressionFailInfo> error;
};

}