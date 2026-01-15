#include <cstddef>
#include <cstdint>
#include <chrono>

namespace ds_pba {
    using usize = std::size_t;

    using i64 = std::int64_t;
    using i32 = std::int32_t;
    using i16 = std::int16_t;

    using u64 = std::uint64_t;
    using u32 = std::uint32_t;
    using u16 = std::uint16_t;

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
}