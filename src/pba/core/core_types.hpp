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

using ObjectId = u32;
inline constexpr ObjectId k_invalid_id{std::numeric_limits<ObjectId>::max()};
inline constexpr usize k_invalid_idx{std::numeric_limits<usize>::max()};

[[nodiscard]] inline ObjectId next_object_id() noexcept
{
    static std::atomic<ObjectId> counter{0};
    const ObjectId id{counter.fetch_add(1u, std::memory_order_relaxed)};
    if (id == k_invalid_id)
    {
        std::println(stderr, "Generated invalid id");
        std::abort();
    }
    return id;
}

template <typename T>
struct Rect
{
    T x{}, y{}, width{}, height{};

    [[nodiscard]] f32 aspect_ratio() const noexcept
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

}  // namespace ds_pba
