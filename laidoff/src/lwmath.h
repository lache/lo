#pragma once
#include "linmath.h"

void calculate_ui_point_from_world_point(const float aspect_ratio,
                                         const mat4x4 proj_view,
                                         const vec4 world_point,
                                         vec2 ui_point);
