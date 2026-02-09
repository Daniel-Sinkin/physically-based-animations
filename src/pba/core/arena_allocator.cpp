// pba/core/arena_allocator.cpp
#include "pba/core/pch.hpp"  // IWYU pragma: keep
//
#include "pba/core/arena_allocator.hpp"
//
#include "pba/core/core_types.hpp"

#include <gsl/assert>

namespace ds_pba
{

ArenaAllocator::ArenaAllocator(usize n_bytes)
    : memory_(std::make_unique<Byte[]>(n_bytes)), capacity_(n_bytes), offset_(0zu)
{
    Expects(n_bytes > 0zu);
}

auto ArenaAllocator::data() const noexcept -> const Byte*
{
    return memory_.get();
}

auto ArenaAllocator::data() noexcept -> Byte*
{
    return memory_.get();
}

auto ArenaAllocator::capacity() const noexcept -> usize
{
    return capacity_;
}

auto ArenaAllocator::used() const noexcept -> usize
{
    return offset_;
}

auto ArenaAllocator::remaining() const noexcept -> usize
{
    return capacity_ - offset_;
}

auto ArenaAllocator::clear() noexcept -> void
{
    offset_ = 0zu;
}

auto ArenaAllocator::allocate(usize n_bytes, usize align) noexcept -> void*
{
    {
        Expects(align > 0zu && "Alignment must be positive.");
        Expects(is_power_of_two(align) && "Alignment must be power of two for allocation");
    }

    offset_ = round_up_aligned(offset_, align);

    if (offset_ + n_bytes > capacity_)
    {
        assert(false && "ArenaAllocator overflow");
        return nullptr;
    }

    void* p = memory_.get() + offset_;
    offset_ += n_bytes;

    {
        Ensures(offset_ <= capacity_);
        return p;
    }
}

auto ArenaAllocator::allocate_array(usize elem_size, usize n_elems, usize align) noexcept -> void*
{
    {
        Expects(elem_size > 0);
        Expects(n_elems > 0);
    }
    return allocate(elem_size * n_elems, align);
}

auto ArenaAllocator::is_valid() const noexcept -> bool
{
    return offset_ <= capacity_;
}

}  // namespace ds_pba
