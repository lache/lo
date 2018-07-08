#include "render_fan.h"
#include "lwcontext.h"
#include "laidoff.h"

void render_fan(const struct _LWCONTEXT *pLwc, const mat4x4 proj, const mat4x4 view,
    float x, float y, float z, float a, float sector_theta, float rscale[FAN_VERTEX_COUNT_PER_ARRAY]) {
    int shader_index = LWST_FAN;

    /*float rscale[FAN_VERTEX_COUNT_PER_ARRAY];
    for (int i = 0; i < FAN_VERTEX_COUNT_PER_ARRAY; i++) {
        rscale[i] = 15.0f;
    }*/

    mat4x4 skin_trans;
    mat4x4_identity(skin_trans);
    mat4x4_translate(skin_trans, x, y, z + 0.01f);
    mat4x4 skin_scale;
    mat4x4_identity(skin_scale);
    mat4x4_scale_aniso(skin_scale, skin_scale, 1, 1, 1);
    mat4x4 skin_rot;
    mat4x4_identity(skin_rot);
    mat4x4_rotate_Z(skin_rot, skin_rot, a - sector_theta / 2/* + (float)LWDEG2RAD(90) + sector_theta / 2*/);

    mat4x4 skin_model;
    mat4x4_identity(skin_model);
    mat4x4_mul(skin_model, skin_rot, skin_model);
    mat4x4_mul(skin_model, skin_scale, skin_model);
    mat4x4_mul(skin_model, skin_trans, skin_model);

    mat4x4 view_model;
    mat4x4 proj_view_model;

    mat4x4_mul(view_model, view, skin_model);
    mat4x4_mul(proj_view_model, proj, view_model);

    LW_FAN_VBO_TYPE lfvt = LFVT_DEFAULT;

    float thetascale = (float)(sector_theta / (2 * M_PI));

    lazy_glUseProgram(pLwc, shader_index);
    glUniform1fv(pLwc->shader[shader_index].rscale_location, FAN_VERTEX_COUNT_PER_ARRAY, rscale);
    glUniform1f(pLwc->shader[shader_index].thetascale_location, thetascale);
    glUniformMatrix4fv(pLwc->shader[shader_index].mvp_location, 1, GL_FALSE, (const GLfloat*)proj_view_model);

    glBindBuffer(GL_ARRAY_BUFFER, pLwc->fan_vertex_buffer[lfvt].vertex_buffer);
    bind_all_fan_vertex_attrib(pLwc, LFVT_DEFAULT);
    glDrawArrays(GL_TRIANGLE_FAN, 0, pLwc->fan_vertex_buffer[lfvt].vertex_count);
}
