// pba/core/arena_allocator.hpp
#pragma once

#include "pba/core/core_types.hpp"

#include <cassert>
#include <cstddef>
#include <gsl/assert>
#include <memory>
#include <span>
#include <type_traits>

namespace ds_pba
{

/**
 * \brief Linear bump allocator for transient simulation/render data.
 *
 * Allocations are served from a single contiguous byte buffer. Individual
 * allocations are not freed; call clear() to rewind the arena.
 */
class ArenaAllocator
{
  public:
    /**
     * \brief Construct an arena with fixed capacity.
     * \param n_bytes Total storage in bytes. Must be greater than zero.
     */
    explicit ArenaAllocator(usize n_bytes);
    ~ArenaAllocator() = default;

    ArenaAllocator(const ArenaAllocator&) = delete;
    ArenaAllocator& operator=(const ArenaAllocator&) = delete;

    ArenaAllocator(ArenaAllocator&&) = delete;
    ArenaAllocator& operator=(ArenaAllocator&&) = delete;

    /**
     * \brief Access the raw backing buffer.
     * \return Pointer to the start of the arena storage.
     */
    [[nodiscard]] auto data() const noexcept -> const Byte*;
    /**
     * \brief Access the raw backing buffer.
     * \return Mutable pointer to the start of the arena storage.
     */
    [[nodiscard]] auto data() noexcept -> Byte*;

    /**
     * \brief Total byte capacity of the arena.
     */
    [[nodiscard]] auto capacity() const noexcept -> usize;
    /**
     * \brief Number of bytes consumed by allocations.
     */
    [[nodiscard]] auto used() const noexcept -> usize;
    /**
     * \brief Number of bytes still available for allocation.
     */
    [[nodiscard]] auto remaining() const noexcept -> usize;

    /**
     * \brief Reset allocation state to the start of the buffer.
     *
     * \note This does not clear old bytes; it only rewinds the write offset.
     */
    auto clear() noexcept -> void;

    /**
     * \brief Allocate an aligned byte range from the arena.
     * \param n_bytes Requested allocation size in bytes.
     * \param align Required alignment in bytes (power of two).
     * \return Pointer to the allocated range, or nullptr on overflow.
     */
    [[nodiscard]] auto allocate(usize n_bytes, usize align) noexcept -> void*;
    /**
     * \brief Allocate an aligned array payload from the arena.
     * \param elem_size Size of one element in bytes.
     * \param n_elems Number of elements to allocate.
     * \param align Required alignment in bytes (power of two).
     * \return Pointer to the allocated range, or nullptr on overflow.
     */
    [[nodiscard]] auto allocate_array(usize elem_size, usize n_elems, usize align) noexcept
        -> void*;

    /**
     * \brief Allocate storage for one value and copy it into the arena.
     * \tparam T Element type. Must be trivially copyable and trivially destructible.
     * \param value Value copied into freshly allocated storage.
     * \return Pointer to copied value in arena memory, or nullptr on overflow.
     */
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

    /**
     * \brief Check whether used bytes stay within arena capacity.
     * \return True when used bytes do not exceed capacity.
     */
    [[nodiscard]] auto is_valid() const noexcept -> bool;

    /**
     * \brief View the used prefix of arena memory as a span of T.
     * \tparam T Element type of the returned view.
     * \return Span over used bytes reinterpreted as T elements.
     * \warning Caller must ensure used() is compatible with sizeof(T) and alignment of T.
     */
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
