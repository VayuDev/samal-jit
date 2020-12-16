#pragma once
#include <cstddef>
#include <functional>
#include <vector>
#include <optional>
#include <any>
#include <map>
#include <cassert>
#include "peg_parser/PegUtil.hpp"
#include "peg_parser/PegForward.hpp"

namespace peg {

struct ParsingState {
  size_t tokenizerState = 0;
};

class Any final {
 public:
  Any() = default;
  template<typename T, std::enable_if_t<!std::is_same<T, Any>::value, bool> = true>
  inline Any(T&& ele) {
    data = new T{std::forward<T>(ele)};
    destructor = [](void *d) {
      delete (T*)d;
    };
  }
  inline ~Any() {
    if(destructor)
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
    auto cpy = data;
    data = nullptr;
    destructor = {};
    return static_cast<T>(cpy);
  }
  template<typename T>
  inline T moveValue() {
    T movedValue = std::move(*static_cast<T*>(data));
    delete data;
    data = nullptr;
    destructor = {};
    return movedValue;
  }
 private:
  void* data = nullptr;
  std::function<void(void*d)> destructor;
};


struct MatchInfo {
  const char* start = nullptr;
  const char* end = nullptr;
  std::optional<size_t> choice;
  Any result;
  std::vector<MatchInfo> subs;

  [[nodiscard]] inline const char* startTrimmed() const {
    auto retStart = start;
    while(*retStart == ' ' && retStart < end) {
      retStart += 1;
    }
    return retStart;
  }
  [[nodiscard]] inline const char* endTrimmed() const {
    auto retEnd = end;
    while(*(retEnd-1) == ' ' && start < retEnd) {
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