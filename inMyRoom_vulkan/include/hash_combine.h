#pragma once

#include <vector>
#include <array>
#include <cstdint>

template <class T>
inline void hash_combine(std::size_t & s, const T & v)
{
    std::hash<T> h;
    s ^= h(v) + 0x9e3779b9 + (s << 6) + (s >> 2);
}

template<typename T>
struct std::hash<std::vector<T>>
{
    inline std::size_t operator()(std::vector<T> const& vec_t) const noexcept
    {
        std::size_t s = vec_t.size();
        for(const auto& t: vec_t) {
            hash_combine(s, t);
        }

        return s;
    }
};

template<typename T, size_t _size>
struct std::hash<std::array<T, _size>>
{
    inline std::size_t operator()(std::array<T, _size> const& array_t) const noexcept
    {
        std::size_t s = array_t.size();
        for(const auto& t: array_t) {
            hash_combine(s, t);
        }

        return s;
    }
};