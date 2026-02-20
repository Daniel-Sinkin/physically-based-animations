// pba/simulation/scenes_table.cpp
#include "pba/core/pch.hpp"  // IWYU pragma: keep
//
#include "pba/simulation/scenes.hpp"
//
#include "pba/simulation/scene_id.hpp"
#include "pba/simulation/simulation_context.hpp"

namespace ds_pba
{
void setup_scene_stable_pyramid_2d_3d(SimulationContext& sim) noexcept;
void setup_scene_projectile_wall(SimulationContext& sim) noexcept;
void setup_scene_cube_cloud_1200(SimulationContext& sim) noexcept;
void setup_scene_orbital_rotor_vortex(SimulationContext& sim) noexcept;
void setup_scene_domino_spiral_cascade(SimulationContext& sim) noexcept;
void setup_scene_tumbler_drum(SimulationContext& sim) noexcept;
void setup_scene_cup_rain_collapse(SimulationContext& sim) noexcept;
void setup_scene_tower_demolition(SimulationContext& sim) noexcept;
void setup_scene_triple_pyramid_siege(SimulationContext& sim) noexcept;
void setup_scene_cube_crossfire_arena(SimulationContext& sim) noexcept;
void setup_scene_nbody_cube_galaxy(SimulationContext& sim) noexcept;
void setup_scene_inclined_avalanche(SimulationContext& sim) noexcept;

namespace
{
using SetupSceneFn = void (*)(SimulationContext&) noexcept;

constexpr usize k_scene_count{static_cast<usize>(SceneId::Count)};

constexpr std::array<SceneMetadata, k_scene_count> k_catalog = {{
    SceneMetadata{
        .id = SceneId::StablePyramid2D3D,
        .name = "Stable Pyramid Showcase (2D + 3D)",
        .description =
            "Side-by-side 2D and 3D pyramid stacks on one ground plane under gravity.",
    },
    SceneMetadata{
        .id = SceneId::ProjectileWall,
        .name = "Projectile vs Wall",
        .description =
            "Brick wall on ground with high-speed projectile impact.",
    },
    SceneMetadata{
        .id = SceneId::CubeCloud1200,
        .name = "Cube Cloud (1200)",
        .description =
            "Exactly 1200 cubes dropped in a container for dense-contact stress testing.",
    },
    SceneMetadata{
        .id = SceneId::OrbitalRotorVortex,
        .name = "Orbital Rotor Vortex",
        .description =
            "No-gravity chamber with motorized rotors and attract/repel fields.",
    },
    SceneMetadata{
        .id = SceneId::DominoSpiralCascade,
        .name = "Domino Spiral Cascade",
        .description =
            "Cube domino path in shrinking loops with single-impact chain reaction.",
    },
    SceneMetadata{
        .id = SceneId::TumblerDrum,
        .name = "Tumbler Drum",
        .description =
            "Dense cube pile stirred by heavy motor-driven paddles inside a rigid container under "
            "gravity.",
    },
    SceneMetadata{
        .id = SceneId::CupRainCollapse,
        .name = "Cup Rain Collapse",
        .description =
            "Cubes rain into a tall cup and collapse under gravity.",
    },
    SceneMetadata{
        .id = SceneId::TowerDemolition,
        .name = "Tower Demolition",
        .description =
            "Tall hollow cube tower hit by high-energy projectiles.",
    },
    SceneMetadata{
        .id = SceneId::TriplePyramidSiege,
        .name = "Triple Pyramid Siege",
        .description =
            "Three pyramid targets with lateral projectiles plus one heavy top drop.",
    },
    SceneMetadata{
        .id = SceneId::CubeCrossfireArena,
        .name = "Cube Crossfire Arena",
        .description =
            "No-gravity projectile crossfire through a central static obstacle lattice.",
    },
    SceneMetadata{
        .id = SceneId::NBodyCubeGalaxy,
        .name = "N-Body Cube Galaxy",
        .description =
            "Force-based orbital setup with a heavy core and multiple rings of orbiters.",
    },
    SceneMetadata{
        .id = SceneId::InclinedAvalanche,
        .name = "Inclined Avalanche",
        .description =
            "Inclined-slope cube release with downstream runout basin.",
    },
}};

constexpr std::array<SetupSceneFn, k_scene_count> k_setup_fns = {{
    &setup_scene_stable_pyramid_2d_3d,
    &setup_scene_projectile_wall,
    &setup_scene_cube_cloud_1200,
    &setup_scene_orbital_rotor_vortex,
    &setup_scene_domino_spiral_cascade,
    &setup_scene_tumbler_drum,
    &setup_scene_cup_rain_collapse,
    &setup_scene_tower_demolition,
    &setup_scene_triple_pyramid_siege,
    &setup_scene_cube_crossfire_arena,
    &setup_scene_nbody_cube_galaxy,
    &setup_scene_inclined_avalanche,
}};

static_assert(k_catalog.size() == k_setup_fns.size());
static_assert(k_catalog.size() == k_scene_count);

constexpr auto index_of_scene_id(SceneId id) noexcept -> std::optional<usize>
{
    for (usize i{0zu}; i < k_catalog.size(); ++i)
    {
        if (k_catalog[i].id == id)
        {
            return i;
        }
    }
    return std::nullopt;
}

constexpr auto catalog_is_consistent() noexcept -> bool
{
    for (usize i{0zu}; i < k_catalog.size(); ++i)
    {
        if (k_setup_fns[i] == nullptr)
        {
            return false;
        }
        for (usize j{i + 1zu}; j < k_catalog.size(); ++j)
        {
            if (k_catalog[i].id == k_catalog[j].id)
            {
                return false;
            }
        }
    }
    return true;
}
static_assert(catalog_is_consistent(), "Scene catalog contains invalid or duplicate entries.");
}  // namespace

auto scene_count() noexcept -> usize
{
    return k_catalog.size();
}

auto scene_index(SceneId id) noexcept -> std::optional<usize>
{
    return index_of_scene_id(id);
}

auto scene_id_from_index(usize index) noexcept -> std::optional<SceneId>
{
    if (index >= k_catalog.size())
    {
        return std::nullopt;
    }
    return k_catalog[index].id;
}

auto scene_metadata(SceneId id) noexcept -> std::optional<SceneMetadata>
{
    const auto idx = index_of_scene_id(id);
    if (!idx.has_value())
    {
        return std::nullopt;
    }
    return k_catalog[*idx];
}

auto scene_catalog() noexcept -> std::span<const SceneMetadata>
{
    return std::span<const SceneMetadata>{k_catalog.data(), k_catalog.size()};
}

auto scene_name(SceneId id) noexcept -> std::string_view
{
    const auto meta = scene_metadata(id);
    return meta.has_value() ? meta->name : "(invalid scene)";
}

auto scene_description(SceneId id) noexcept -> std::string_view
{
    const auto meta = scene_metadata(id);
    return meta.has_value() ? meta->description : "(invalid scene)";
}

void setup_scene_by_id(SimulationContext& sim, SceneId id) noexcept
{
    const auto idx = scene_index(id);
    if (!idx.has_value())
    {
        return;
    }

    const auto setup_fn = k_setup_fns[*idx];
    Expects(setup_fn != nullptr);
    if (setup_fn)
    {
        setup_fn(sim);
    }
}

}  // namespace ds_pba
