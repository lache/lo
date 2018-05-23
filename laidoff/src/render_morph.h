#pragma once

struct _LWCONTEXT;

void render_morph(const LWCONTEXT* pLwc,
                  GLuint tex_index,
                  LW_MORPH_VBO_TYPE lmvt,
                  float alpha_multiplier,
                  float over_r,
                  float over_g,
                  float over_b,
                  float oratio,
                  const mat4x4 proj,
                  const mat4x4 view,
                  const mat4x4 model,
                  const float* uv_offset,
                  const float* uv_scale,
                  const float earth_globe_morph_weight);
