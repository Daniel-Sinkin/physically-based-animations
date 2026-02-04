// pba/core/arena_allocator.hpp
#pragma once

#include "pba/core/core_types.hpp"

#include <cassert>
#include <cstddef>
#include <memory>
#include <span>
#include <type_traits>

namespace ds_pba
{

class ArenaAllocator
{
  public:
    explicit ArenaAllocator(usize n_bytes);
    ~ArenaAllocator() = default;

    ArenaAllocator(const ArenaAllocator&) = delete;
    ArenaAllocator& operator=(const ArenaAllocator&) = delete;

    ArenaAllocator(ArenaAllocator&&) = delete;
    ArenaAllocator& operator=(ArenaAllocator&&) = delete;

    [[nodiscard]] auto data() const noexcept -> const Byte*;
    [[nodiscard]] auto data() noexcept -> Byte*;

    [[nodiscard]] auto capacity() const noexcept -> usize;
    [[nodiscard]] auto used() const noexcept -> usize;
    [[nodiscard]] auto remaining() const noexcept -> usize;

    auto clear() noexcept -> void;

    [[nodiscard]] auto allocate(usize n_bytes, usize align) noexcept -> void*;

    template <class T>
        requires(std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T>)
    [[nodiscard]] auto push_back(const T& value) noexcept -> T*
    {
        void* mem = allocate(sizeof(T), alignof(T));
        if (!mem)
        {
            return nullptr;
        }

        *static_cast<T*>(mem) = value;
        return static_cast<T*>(mem);
    }

    [[nodiscard]] auto is_valid() const noexcept -> bool;

    template <typename T>
    [[nodiscard]] auto as_span() noexcept -> std::span<T>
    {
        return std::span<T>{reinterpret_cast<T*>(memory_.get()), offset_ / sizeof(T)};
    }

  private:
    std::unique_ptr<Byte[]> memory_{};
    usize capacity_{0zu};
    usize offset_{0zu};
};

}  // namespace ds_pba
