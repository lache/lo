#include "lwmath.h"
#include "lwcontext.h"

void calculate_ui_point_from_world_point(const float aspect_ratio,
                                         const mat4x4 proj_view,
                                         const vec4 world_point,
                                         vec2 ui_point) {

	vec4 clip_space_point;
	mat4x4_mul_vec4(clip_space_point, proj_view, world_point);

	const vec3 ndc_pos = {
		clip_space_point[0] / clip_space_point[3],
		clip_space_point[1] / clip_space_point[3],
		clip_space_point[2] / clip_space_point[3]
	};

    float rt_x, rt_y;
    lwcontext_rt_corner(aspect_ratio, &rt_x, &rt_y);
	ui_point[0] = ndc_pos[0] * rt_x;
	ui_point[1] = ndc_pos[1] * rt_y;
}
