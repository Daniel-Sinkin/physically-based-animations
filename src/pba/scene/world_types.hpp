// pba/scene/scene_types.hpp
#pragma once

#include "pba/core/arena_allocator.hpp"
#include "pba/core/geometry.hpp"
#include "pba/core/math_types.hpp"
//
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <gsl/assert>
#include <optional>

namespace ds_pba
{

struct Transform
{
    Pos3 position{};
    Dir3 scale{1.0f, 1.0f, 1.0f};
    Quaternion orientation{k_quaternion_identity};

    [[nodiscard]] auto model_matrix() const noexcept -> ModelMatrix
    {
        auto M{glm::identity<glm::mat4>()};
        M = glm::translate(M, position);
        M *= glm::mat4_cast(orientation);
        M = glm::scale(M, scale);
        return ModelMatrix{M};
    }
};

struct TransformSOA
{
    explicit TransformSOA(usize capacity) : memory_(bytes_needed(capacity)), capacity_(capacity)
    {
        allocate_arrays();
    }
    ~TransformSOA() = default;

    auto reset() -> void
    {
        size_ = 0zu;
    }

    [[nodiscard]] auto set(usize idx, const Transform& t) noexcept -> bool
    {
        if (idx >= capacity_)
        {
            assert(false);
            return false;
        }
        if (idx >= size_)
        {
            return false;
        }
        pos_x_[idx] = t.position.x;
        pos_y_[idx] = t.position.y;
        pos_z_[idx] = t.position.z;
        scale_x_[idx] = t.scale.x;
        scale_y_[idx] = t.scale.y;
        scale_z_[idx] = t.scale.z;
        orientation_[idx] = t.orientation;

        return true;
    }

    auto get(usize idx) -> std::optional<Transform>
    {
        if (idx >= size_)
        {
            assert(false && "build_transform_from_idx OOB");
            return std::nullopt;
        }

        return Transform{
            .position = Pos3{pos_x_[idx], pos_y_[idx], pos_z_[idx]},
            .scale = Dir3{scale_x_[idx], scale_y_[idx], scale_z_[idx]},
            .orientation = orientation_[idx]
        };
    }

    auto push_back(const Transform& t) -> std::optional<usize>
    {
        if (size_ >= capacity_)
        {
            return std::nullopt;
        }
        const auto back = size_;
        ++size_;
        if (!set(back, t))
        {
            return std::nullopt;
        }
        else
        {
            return back;
        }
    }
    // clang-format off
    [[nodiscard]] auto pos_x()       noexcept -> std::span<f32      > { return {pos_x_, size_}; }
    [[nodiscard]] auto pos_x() const noexcept -> std::span<const f32> { return {pos_x_, size_}; }
    [[nodiscard]] auto pos_y()       noexcept -> std::span<f32      > { return {pos_y_, size_}; }
    [[nodiscard]] auto pos_y() const noexcept -> std::span<const f32> { return {pos_y_, size_}; }
    [[nodiscard]] auto pos_z()       noexcept -> std::span<f32      > { return {pos_z_, size_}; }
    [[nodiscard]] auto pos_z() const noexcept -> std::span<const f32> { return {pos_z_, size_}; }

    [[nodiscard]] auto scale_x()       noexcept -> std::span<f32      > { return {scale_x_, size_}; }
    [[nodiscard]] auto scale_x() const noexcept -> std::span<const f32> { return {scale_x_, size_}; }
    [[nodiscard]] auto scale_y()       noexcept -> std::span<f32      > { return {scale_y_, size_}; }
    [[nodiscard]] auto scale_y() const noexcept -> std::span<const f32> { return {scale_y_, size_}; }
    [[nodiscard]] auto scale_z()       noexcept -> std::span<f32      > { return {scale_z_, size_}; }
    [[nodiscard]] auto scale_z() const noexcept -> std::span<const f32> { return {scale_z_, size_}; }

    [[nodiscard]] auto orientation()       noexcept -> std::span<Quaternion      > { return {orientation_, size_}; }
    [[nodiscard]] auto orientation() const noexcept -> std::span<const Quaternion> { return {orientation_, size_}; }
    // clang-format on

  private:
    ArenaAllocator memory_;
    usize capacity_{};
    usize size_{};

    f32* pos_x_{};
    f32* pos_y_{};
    f32* pos_z_{};
    f32* scale_x_{};
    f32* scale_y_{};
    f32* scale_z_{};
    Quaternion* orientation_{};

    static constexpr auto bytes_needed(usize n) -> usize
    {  // Computes how much memory the allocator will use (with some slack for safety)
        constexpr usize k_slack{64zu};
        return n * (6 * sizeof(f32) + sizeof(Quaternion)) + k_slack;
    };

    auto allocate_arrays() -> void
    {
        {
            Expects(capacity_ > 0);
        }
        // clang-format off
        pos_x_       = static_cast<f32*>       (memory_.allocate_array(sizeof(f32)       , capacity_, alignof(f32)));
        pos_y_       = static_cast<f32*>       (memory_.allocate_array(sizeof(f32)       , capacity_, alignof(f32)));
        pos_z_       = static_cast<f32*>       (memory_.allocate_array(sizeof(f32)       , capacity_, alignof(f32)));
        scale_x_     = static_cast<f32*>       (memory_.allocate_array(sizeof(f32)       , capacity_, alignof(f32)));
        scale_y_     = static_cast<f32*>       (memory_.allocate_array(sizeof(f32)       , capacity_, alignof(f32)));
        scale_z_     = static_cast<f32*>       (memory_.allocate_array(sizeof(f32)       , capacity_, alignof(f32)));
        orientation_ = static_cast<Quaternion*>(memory_.allocate_array(sizeof(Quaternion), capacity_, alignof(Quaternion)));
        // clang-format on
    }
};

}  // namespace ds_pba
