// pba/core/core_types.hpp
#pragma once

#include <array>
#include <assert.h>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <print>

namespace ds_pba
{
using usize = std::size_t;
using isize = std::ptrdiff_t;
using uptr = std::uintptr_t;

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

inline constexpr usize k_invalid_idx{std::numeric_limits<usize>::max()};

template <typename T>
struct Rect
{
    T x{}, y{}, w{}, h{};

    [[nodiscard]] f32 aspect_ratio() const noexcept
    {
        if (h == 0)
        {
            return 1.0f;
        }
        return static_cast<f32>(w) / static_cast<f32>(h);
    }
};

using RectInt = Rect<int>;
using RectF32 = Rect<f32>;
using RectF64 = Rect<f64>;

// clang-format off
template <typename T>
struct ColorRGBA
{
    std::array<T, 4> v{};

    constexpr ColorRGBA() = default;
    constexpr ColorRGBA(T r, T g, T b, T a) noexcept : v{r, g, b, a} {}

    constexpr T*       data()       noexcept { return v.data(); }
    constexpr const T* data() const noexcept { return v.data(); }

    constexpr T&          r()       noexcept { return v[0]; }
    constexpr T&          g()       noexcept { return v[1]; }
    constexpr T&          b()       noexcept { return v[2]; }
    constexpr T&          a()       noexcept { return v[3]; }

    constexpr const T&    r() const noexcept { return v[0]; }
    constexpr const T&    g() const noexcept { return v[1]; }
    constexpr const T&    b() const noexcept { return v[2]; }
    constexpr const T&    a() const noexcept { return v[3]; }
};
// clang-format on

using ColorRGBA8 = ColorRGBA<u8>;
using ColorRGBAf = ColorRGBA<f32>;
using ColorRGBAd = ColorRGBA<f64>;

// clang-format off
struct Color3
{
    std::array<f32, 3> v{};

    constexpr Color3() noexcept = default;
    constexpr Color3(f32 r, f32 g, f32 b) noexcept : v{r, g, b} {}

    constexpr f32*       data()       noexcept { return v.data(); }
    constexpr const f32* data() const noexcept { return v.data(); }

    constexpr f32&       r()       noexcept { return v[0]; }
    constexpr f32&       g()       noexcept { return v[1]; }
    constexpr f32&       b()       noexcept { return v[2]; }

    constexpr const f32& r() const noexcept { return v[0]; }
    constexpr const f32& g() const noexcept { return v[1]; }
    constexpr const f32& b() const noexcept { return v[2]; }

    static const Color3 Black;
    static const Color3 White;
    static const Color3 Red;
    static const Color3 Green;
    static const Color3 Blue;
    static const Color3 Yellow;
    static const Color3 Cyan;
    static const Color3 Magenta;
    static const Color3 Orange;
    static const Color3 Purple;
    static const Color3 Gray;
};
inline constexpr Color3 Color3::Black   {0.0f, 0.0f, 0.0f};
inline constexpr Color3 Color3::White   {1.0f, 1.0f, 1.0f};
inline constexpr Color3 Color3::Red     {1.0f, 0.0f, 0.0f};
inline constexpr Color3 Color3::Green   {0.0f, 1.0f, 0.0f};
inline constexpr Color3 Color3::Blue    {0.0f, 0.0f, 1.0f};
inline constexpr Color3 Color3::Yellow  {1.0f, 1.0f, 0.0f};
inline constexpr Color3 Color3::Cyan    {0.0f, 1.0f, 1.0f};
inline constexpr Color3 Color3::Magenta {1.0f, 0.0f, 1.0f};
inline constexpr Color3 Color3::Orange  {1.0f, 0.5f, 0.0f};
inline constexpr Color3 Color3::Purple  {0.5f, 0.0f, 0.5f};
inline constexpr Color3 Color3::Gray    {0.5f, 0.5f, 0.5f};
// clang-format on
}  // namespace ds_pba
