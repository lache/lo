#include "lwvertexshader.h"
#include "lwlog.h"
#include "lwgl.h"
#include "file.h"
#include "lwcontext.h"
#include <stdlib.h>

GLuint lw_create_vertex_shader(LW_VERTEX_SHADER lwvs, const char* path) {
    char* vst = create_string_from_file(path);
    if (!vst) {
        LOGEP("file not found: %s", path);
        return -1;
    }
    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    const GLchar* vstarray[] = { LW_GLSL_VERSION_STATEMENT, vst };
    glShaderSource(vertex_shader, 2, vstarray, NULL);
    glCompileShader(vertex_shader);
    GLint isCompiled = 0;
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &isCompiled);
    if (isCompiled == GL_FALSE) {
        GLint maxLength = 0;
        glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &maxLength);
        // The maxLength includes the NULL character
        GLchar *errorLog = (GLchar *)calloc(maxLength, sizeof(GLchar));
        glGetShaderInfoLog(vertex_shader, maxLength, &maxLength, errorLog);
        LOGEP("LW_VERTEX_SHADER [%d] Error (length:%d): %s", lwvs, maxLength, errorLog);
        free(errorLog);
        // Provide the info log in whatever manor you deem best.
        // Exit with failure.
        glDeleteShader(vertex_shader); // Don't leak the shader.
        release_string(vst);
        return -2;
    }
    release_string(vst);
    return vertex_shader;
}

void lw_create_all_vertex_shader(LWCONTEXT* pLwc) {
    for (int i = 0; i < LWVS_COUNT; i++) {
        pLwc->vertex_shader[i] = lw_create_vertex_shader(LWVS_DEFAULT + i, vertex_shader_filename[i]);
    }
}

void lw_delete_all_vertex_shader(LWCONTEXT* pLwc) {
    for (int i = 0; i < LWVS_COUNT; i++) {
        glDeleteShader(pLwc->vertex_shader[i]);
    }
}
