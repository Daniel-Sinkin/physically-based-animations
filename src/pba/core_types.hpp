// pba/core_types.hpp
#pragma once

#include <array>
#include <assert.h>
#include <chrono>
#include <cstddef>
#include <cstdint>

namespace ds_pba
{
using usize = std::size_t;

using i64 = std::int64_t;
using i32 = std::int32_t;
using i16 = std::int16_t;
using i8 = std::int8_t;

using u64 = std::uint64_t;
using u32 = std::uint32_t;
using u16 = std::uint16_t;
using u8 = std::uint8_t;

#if defined(__cpp_lib_stdfloat) && __cpp_lib_stdfloat >= 202207L
using f32 = std::float32_t;
using f64 = std::float64_t;
#else
using f32 = float;
using f64 = double;
#endif
static_assert(sizeof(f32) == 4);
static_assert(sizeof(f64) == 8);

using Clock = std::chrono::steady_clock;
using TimePoint = Clock::time_point;
using Duration = std::chrono::duration<f64>;

using ObjectId = u32;

inline ObjectId next_object_id()
{
    static ObjectId counter{0u};
    return counter++;
}

template <typename T>
struct Rect
{
    T x{}, y{}, width{}, height{};

    f32 aspect_ratio() const noexcept
    {
        if (height == 0)
        {
            return 1.0f;
        }
        return static_cast<f32>(width) / static_cast<f32>(height);
    }
};

using RectInt = Rect<int>;
using RectF32 = Rect<f32>;
using RectF64 = Rect<f64>;

template <typename T>
struct ColorRGBA
{
    std::array<T, 4> v{};

    constexpr ColorRGBA() = default;
    constexpr ColorRGBA(T r, T g, T b, T a) noexcept : v{r, g, b, a}
    {
    }

    constexpr T* data() noexcept
    {
        return v.data();
    }
    constexpr const T* data() const noexcept
    {
        return v.data();
    }

    constexpr T& r() noexcept
    {
        return v[0];
    }
    constexpr T& g() noexcept
    {
        return v[1];
    }
    constexpr T& b() noexcept
    {
        return v[2];
    }
    constexpr T& a() noexcept
    {
        return v[3];
    }

    constexpr const T& r() const noexcept
    {
        return v[0];
    }
    constexpr const T& g() const noexcept
    {
        return v[1];
    }
    constexpr const T& b() const noexcept
    {
        return v[2];
    }
    constexpr const T& a() const noexcept
    {
        return v[3];
    }
};

using ColorRGBA8 = ColorRGBA<u8>;
using ColorRGBAf = ColorRGBA<f32>;
using ColorRGBAd = ColorRGBA<f64>;

}  // namespace ds_pba
