#include "lwgl.h"
#include "lwshader.h"
#include "lwcontext.h"

void lazy_glUseProgram(const LWCONTEXT* pLwc, int/*LW_SHADER_TYPE*/ shader_index) {
    lw_create_lazy_shader_program(pLwc, shader_index);
    glUseProgram(pLwc->shader[shader_index].program);
}
