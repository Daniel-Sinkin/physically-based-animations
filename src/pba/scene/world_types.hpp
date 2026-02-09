// pba/scene/scene_types.hpp
#pragma once

#include "pba/core/arena_allocator.hpp"
#include "pba/core/geometry.hpp"
#include "pba/core/math_types.hpp"
//
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
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
    explicit TransformSOA(usize n_elems) : memory_(n_elems), n_elems_(n_elems)
    {
        auto align = alignof(Transform);
        // clang-format off
        pos_x_       = static_cast<f32*>       (memory_.allocate_array(sizeof(f32)       , n_elems_, align));
        pos_y_       = static_cast<f32*>       (memory_.allocate_array(sizeof(f32)       , n_elems_, align));
        pos_z_       = static_cast<f32*>       (memory_.allocate_array(sizeof(f32)       , n_elems_, align));
        scale_x_     = static_cast<f32*>       (memory_.allocate_array(sizeof(f32)       , n_elems_, align));
        scale_y_     = static_cast<f32*>       (memory_.allocate_array(sizeof(f32)       , n_elems_, align));
        scale_z_     = static_cast<f32*>       (memory_.allocate_array(sizeof(f32)       , n_elems_, align));
        orientation_ = static_cast<Quaternion*>(memory_.allocate_array(sizeof(Quaternion), n_elems_, align));
        // clang-format on
    }
    ~TransformSOA() = default;

    auto build_transform_from_idx(usize idx) const noexcept -> std::optional<Transform>
    {
        if (idx >= n_elems_)
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

  private:
    ArenaAllocator memory_;

    f32* pos_x_;
    f32* pos_y_;
    f32* pos_z_;
    f32* scale_x_;
    f32* scale_y_;
    f32* scale_z_;
    Quaternion* orientation_;

    usize n_elems_;
};

}  // namespace ds_pba
