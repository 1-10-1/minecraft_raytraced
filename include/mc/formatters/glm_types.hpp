#pragma once

#include <concepts>

#include <fmt/core.h>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/ext/vector_int2.hpp>
#include <glm/ext/vector_int3.hpp>
#include <glm/ext/vector_int4.hpp>
#include <glm/ext/vector_uint2.hpp>
#include <glm/ext/vector_uint3.hpp>
#include <glm/ext/vector_uint4.hpp>

// Concept for vector-like classes
template<typename VecType>
concept VectorType = requires(VecType v) {
    {
        v.x
    } -> std::convertible_to<float>;
};

// Vector dimension class
template<VectorType VecType>
struct VectorDimension;

template<VectorType VecType>
struct VectorDimension
{
    static constexpr int value = 1;  // Default to 1D
};

template<VectorType VecType>
    requires requires(VecType v) {
        {
            v.y
        } -> std::convertible_to<float>;
    }
struct VectorDimension<VecType>
{
    static constexpr int value = 2;  // Detected .y member, so 2D
};

template<VectorType VecType>
    requires requires(VecType v) {
        {
            v.z
        } -> std::convertible_to<float>;
    }
struct VectorDimension<VecType>
{
    static constexpr int value = 3;  // Detected .z member, so 3D
};

template<VectorType VecType>
    requires requires(VecType v) {
        {
            v.w
        } -> std::convertible_to<float>;
    }
struct VectorDimension<VecType>
{
    static constexpr int value = 4;  // Detected .w member, so 4D
};

template<typename VecType>
struct VecFormatter
{
    constexpr auto parse(fmt::format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(VecType const& v, FormatContext& ctx)
    {
        int dim = VectorDimension<VecType>::value;

        fmt::format_to(ctx.out(), "({}", v[0]);

        for (int i = 1; i < dim; ++i)
        {
            fmt::format_to(ctx.out(), ",{}", v[i]);
        }
        return fmt::format_to(ctx.out(), ")");
    }
};

template<>
struct fmt::formatter<glm::vec2> : VecFormatter<glm::vec2>
{
};

template<>
struct fmt::formatter<glm::vec3> : VecFormatter<glm::vec3>
{
};

template<>
struct fmt::formatter<glm::vec4> : VecFormatter<glm::vec4>
{
};

template<>
struct fmt::formatter<glm::uvec2> : VecFormatter<glm::uvec2>
{
};

template<>
struct fmt::formatter<glm::uvec3> : VecFormatter<glm::uvec3>
{
};

template<>
struct fmt::formatter<glm::uvec4> : VecFormatter<glm::uvec4>
{
};

template<>
struct fmt::formatter<glm::ivec2> : VecFormatter<glm::ivec2>
{
};

template<>
struct fmt::formatter<glm::ivec3> : VecFormatter<glm::ivec3>
{
};

template<>
struct fmt::formatter<glm::ivec4> : VecFormatter<glm::ivec4>
{
};
