#pragma once

#include <random>

class Random
{
public:
    Random() = delete;

    static auto getGen() -> auto const& { return m_gen; }

    template<typename T>
    static auto between(T a, T b) -> T
    {
        static_assert(std::is_integral_v<T> || std::is_floating_point_v<T>,
                      "Between expected either integer or float params.");

        if constexpr (std::is_floating_point_v<T>)
        {
            return std::uniform_real_distribution<T> { a, b }(m_gen);
        }
        else if constexpr (std::is_integral_v<T>)
        {
            return std::uniform_int_distribution<T> { a, b }(m_gen);
        }
    }

private:
    inline static const std::mt19937_64 m_gen { std::random_device {}() };
};
