#pragma once

#include <algorithm>

template<size_t N>
struct FixedString {
    constexpr FixedString(const char(&str)[N]) {
        std::copy_n(str, N - 1, value);
    }
    constexpr const char* data() const { return value; }
    static constexpr size_t size() { return N - 1; }

    char value[N - 1];
};