#include "lwvbo.h"
#include "lwvbotype.h"
#include "file.h"
#include "lwcontext.h"
#include <assert.h>
#include "laidoff.h"

void lw_load_vbo_data(LWCONTEXT* pLwc, const char* mesh_vbo_data, size_t mesh_size, LWVBO* pVbo, int stride_in_bytes) {
    GLuint vbo = 0;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, mesh_size, mesh_vbo_data, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    pVbo->vertex_buffer = vbo;
    pVbo->vertex_count = (int)(mesh_size / stride_in_bytes);
}

static void lw_load_vbo_meta_data(LWCONTEXT* pLwc, const char* mesh_vbo_meta_data, LWVBO* pVbo) {
    if (mesh_vbo_meta_data) {
        pVbo->bound_valid = 1;
        memcpy(pVbo->bound_min, mesh_vbo_meta_data + sizeof(float) * 0, sizeof(float) * 3);
        memcpy(pVbo->bound_max, mesh_vbo_meta_data + sizeof(float) * 3, sizeof(float) * 3);
    }
}

void lw_load_vbo(LWCONTEXT* pLwc, const char* filename, LWVBO* pVbo, int stride_in_bytes) {
    // load VBO file itself
    size_t mesh_file_size = 0;
    char *mesh_vbo_data = create_binary_from_file(filename, &mesh_file_size);
    lw_load_vbo_data(pLwc, mesh_vbo_data, mesh_file_size, pVbo, stride_in_bytes);
    release_binary(mesh_vbo_data);
    // load VBO metadata
    char filename_meta[1024] = { 0, };
    strncat(filename_meta, filename, sizeof(filename_meta));
    strncat(filename_meta, ".meta", sizeof(filename_meta));
    filename_meta[sizeof(filename_meta) - 1] = 0;
    size_t mesh_meta_file_size = 0;
    char *mesh_vbo_meta_data = create_binary_from_file(filename_meta, &mesh_meta_file_size);
    lw_load_vbo_meta_data(pLwc, mesh_vbo_meta_data, pVbo);
    release_binary(mesh_vbo_meta_data);
}

void lw_load_all_vbo(LWCONTEXT* pLwc) {
    for (int i = 0; i < LVT_COUNT; i++) {
        if (vbo_filename[i].filename[0] != '\0') {
            int stride_in_bytes = 0;
            if (vbo_filename[i].shader_index != LWST_DEFAULT_NORMAL_COLOR) {
                stride_in_bytes = lwvertex_stride_in_bytes;
            } else {
                stride_in_bytes = lwcolorvertex_stride_in_bytes;
            }
            lw_load_vbo(pLwc, vbo_filename[i].filename, &pLwc->vertex_buffer[i], stride_in_bytes);
        }
    }
}

static void lw_lazy_load_vbo(LWCONTEXT* pLwc, LW_VBO_TYPE lvt) {
    LWVBO* vbo = &pLwc->vertex_buffer[lvt];
    if (vbo->vertex_buffer) {
        return;
    }
    int stride_in_bytes = 0;
    if (vbo_filename[lvt].shader_index != LWST_DEFAULT_NORMAL_COLOR) {
        stride_in_bytes = lwvertex_stride_in_bytes;
    } else {
        stride_in_bytes = lwcolorvertex_stride_in_bytes;
    }
    lw_load_vbo(pLwc, vbo_filename[lvt].filename, &pLwc->vertex_buffer[lvt], stride_in_bytes);
}

void lw_setup_vao(LWCONTEXT* pLwc, int lvt) {
// Vertex Array Objects
#if LW_SUPPORT_VAO
    if (pLwc->vao_ready[lvt] == 0) {
        assert(pLwc->vao[lvt]);
        glBindVertexArray(pLwc->vao[lvt]);
        assert(pLwc->vertex_buffer[lvt].vertex_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, pLwc->vertex_buffer[lvt].vertex_buffer);
        //assert(pLwc->shader[vbo_filename[lvt].shader_index].program);
		if (vbo_filename[lvt].shader_index != LWST_DEFAULT_NORMAL_COLOR) {
			set_vertex_attrib_pointer(pLwc, vbo_filename[lvt].shader_index);
		} else {
			set_color_vertex_attrib_pointer(pLwc, vbo_filename[lvt].shader_index);
		}
        pLwc->vao_ready[lvt] = 1;
	}
#endif
}

void lazy_glBindBuffer(const LWCONTEXT* _pLwc, int lvt) {
    LWCONTEXT* pLwc = (LWCONTEXT*)_pLwc;
    lw_lazy_load_vbo(pLwc, lvt);
    lw_setup_vao(pLwc, lvt);
    glBindBuffer(GL_ARRAY_BUFFER, pLwc->vertex_buffer[lvt].vertex_buffer);
}
