// pba/physics/collision.hpp
#pragma once

#include "pba/core/arena_allocator.hpp"
#include "pba/physics/physics_types.hpp"

#include <array>
#include <vector>

namespace ds_pba
{
struct CollisionPair
{
    usize a_idx{};
    usize b_idx{};
};

struct CollisionScratch
{
    std::vector<f32> aabb_min_x{};
    std::vector<f32> aabb_min_y{};
    std::vector<f32> aabb_min_z{};
    std::vector<f32> aabb_max_x{};
    std::vector<f32> aabb_max_y{};
    std::vector<f32> aabb_max_z{};

    std::vector<u8> is_static{};
    std::vector<std::array<Dir3, 3>> axes{};
    std::vector<std::array<Pos3, 8>> corners{};
    std::vector<usize> sort_order{};
    std::vector<usize> active{};
    std::vector<CollisionPair> candidate_pairs{};

    auto prepare(usize body_count) -> void;
};

struct CollisionStats
{
    usize body_count{};
    usize broadphase_candidates{};
    usize narrowphase_pairs{};
    usize contacts_generated{};
};

[[nodiscard]] auto make_contact_key(EntityId a_id, EntityId b_id, const Pos3& p) noexcept
    -> ContactKey;

[[nodiscard]] auto
generate_obb_contacts(
    const RigidBodySOA& bodies,
    ArenaAllocator& out,
    CollisionScratch& scratch,
    bool collect_stats = true
) -> CollisionStats;
}  // namespace ds_pba
