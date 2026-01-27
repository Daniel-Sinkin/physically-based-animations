// pba/engine/scenes.hpp
#pragma once

#include "pba/core/core_types.hpp"

namespace ds_pba
{
struct EngineContext;

void setup_active_scene(EngineContext& e);
void update_active_scene(EngineContext& e, f32 frame_dt_s);

}  // namespace ds_pba
