// pba/scene_context.cpp
#include "pba/pch.hpp"
//
#include "pba/scene_context.hpp"
//

void ds_pba::SceneContext::setup()
{
    if (cube_objects.empty())
    {
        cube_objects.push_back(
            Object{
                .id = next_object_id(),
                .type = ObjectType::Cube,
                .transform =
                    Transform{
                        .position = {2.0f, 1.0f, 0.5f},
                        .rotation_deg = {0, 0, 0},
                        .scale = {1, 1, 1}
                    },
                .color = {0.85f, 0.35f, 0.25f},
            }
        );
        cube_objects.push_back(
            Object{
                .id = next_object_id(),
                .type = ObjectType::Cube,
                .transform =
                    Transform{
                        .position = {-1.5f, 2.5f, 0.5f},
                        .rotation_deg = {0, 0, 25},
                        .scale = {1, 1, 1}
                    },
                .color = {0.25f, 0.55f, 0.90f},
            }
        );
        cube_objects.push_back(
            Object{
                .id = next_object_id(),
                .type = ObjectType::Cube,
                .transform =
                    Transform{
                        .position = {-2.5f, -1.5f, 0.75f},
                        .rotation_deg = {15, 0, 0},
                        .scale = {1.5f, 1.0f, 1.5f}
                    },
                .color = {0.30f, 0.85f, 0.45f},
            }
        );
        cube_objects.push_back(
            Object{
                .id = next_object_id(),
                .type = ObjectType::Cube,
                .transform =
                    Transform{
                        .position = {-0.5f, -1.5f, 0.75f},
                        .rotation_deg = {15, 0, 0},
                        .scale = {1.5f, 1.0f, 1.5f}
                    },
                .color = {0.30f, 0.85f, 0.45f},
            }
        );
        cube_objects.push_back(
            Object{
                .id = next_object_id(),
                .type = ObjectType::Cube,
                .transform =
                    Transform{
                        .position = {-4.5f, -1.5f, 0.75f},
                        .rotation_deg = {15, 0, 0},
                        .scale = {1.5f, 1.0f, 1.5f}
                    },
                .color = {1.0f, 0.85f, 0.45f},
            }
        );
        cube_objects.push_back(
            Object{
                .id = next_object_id(),
                .type = ObjectType::Cube,
                .transform =
                    Transform{
                        .position = {-10.5f, -1.5f, 0.75f},
                        .rotation_deg = {15, 0, 0},
                        .scale = {1.5f, 1.0f, 1.5f}
                    },
                .color = {1.0f, 0.85f, 0.6f},
            }
        );
        sphere_objects.push_back(
            Object{
                .id = next_object_id(),
                .type = ObjectType::Sphere,
                .transform = Transform{},
                .color = {1.0f, 0.6f, 0.3f},
            }
        );
        camera.pivot = {0.0f, 0.0f, 0.0f};
        camera.distance = 10.0f;
        selected_index = std::nullopt;
        selected_type = std::nullopt;
    }
}
