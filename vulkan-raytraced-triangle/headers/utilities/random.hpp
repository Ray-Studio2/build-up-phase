#pragma once

#include <random>

namespace Random {
    static std::random_device rd;
    static std::mt19937       engine(rd());

    template <typename T>
    inline T range(T from, T to) noexcept {
        std::uniform_real_distribution<T> dis(from, to);

        return dis(engine);
    }
}