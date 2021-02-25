#pragma once
#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace samal {

template<typename T>
using sp = std::shared_ptr<T>;

template<typename T>
using up = std::unique_ptr<T>;

template<typename T>
using wp = std::weak_ptr<T>;

class Stopwatch final {
public:
    explicit Stopwatch(const char* name) {
        mStart = std::chrono::high_resolution_clock::now();
        mName = name;
    }
    void stop() {
        if(mStopped)
            return;
        auto duration = (std::chrono::high_resolution_clock::now() - mStart).count();
        if(strlen(mName) > 0) {
            printf("%s took %0.2fµs\n", mName, duration / 1000.0);
        } else {
            printf("Something took %0.2f\nµs", duration / 1000.0);
        }
        mStopped = true;
    }
    ~Stopwatch() {
        stop();
    }

private:
    const char* mName;
    std::chrono::high_resolution_clock::time_point mStart;
    bool mStopped = false;
};

class CompilationException : public std::exception {
public:
    explicit CompilationException(std::string msg)
    : mMsg(std::move(msg)) {
    }
    [[nodiscard]] const char* what() const noexcept override {
        return mMsg.c_str();
    }

private:
    std::string mMsg;
};

static inline std::string concat(const std::vector<std::string>& splitName) {
    std::string ret;
    for(size_t i = 0; i < splitName.size(); ++i) {
        ret += splitName.at(i);
        if(i < splitName.size() - 1) {
            ret += ".";
        }
    }
    return ret;
}

class DestructorWrapper {
public:
    explicit inline DestructorWrapper(std::function<void()> callback) {
        mCallback = std::move(callback);
        assert(mCallback);
    }
    inline ~DestructorWrapper() {
        mCallback();
    }
    DestructorWrapper(const DestructorWrapper&) = delete;
    DestructorWrapper(DestructorWrapper&&) = delete;
    void operator=(const DestructorWrapper&) = delete;
    void operator=(DestructorWrapper&&) = delete;

private:
    std::function<void()> mCallback;
};

#define todo() assert(false)

}