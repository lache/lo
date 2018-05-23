#include "lwfragshader.h"
#include "lwlog.h"
#include "lwmacro.h"
#include "lwgl.h"
#include "file.h"
#include "lwcontext.h"
#include <stdlib.h>

GLuint lw_create_frag_shader(LW_FRAG_SHADER lwfs, const char* path) {
    char* vst = create_string_from_file(path);
    if (!vst) {
        LOGEP("file not found: %s", path);
        return -1;
    }
    GLuint frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
    const GLchar* vstarray[] = { LW_GLSL_VERSION_STATEMENT, vst };
    glShaderSource(frag_shader, 2, vstarray, NULL);
    glCompileShader(frag_shader);
    GLint isCompiled = 0;
    glGetShaderiv(frag_shader, GL_COMPILE_STATUS, &isCompiled);
    if (isCompiled == GL_FALSE) {
        GLint maxLength = 0;
        glGetShaderiv(frag_shader, GL_INFO_LOG_LENGTH, &maxLength);
        // The maxLength includes the NULL character
        GLchar *errorLog = (GLchar *)calloc(maxLength, sizeof(GLchar));
        glGetShaderInfoLog(frag_shader, maxLength, &maxLength, errorLog);
        LOGEP("LW_FRAG_SHADER [%d] Error (length:%d): %s", lwfs, maxLength, errorLog);
        free(errorLog);
        // Provide the info log in whatever manor you deem best.
        // Exit with failure.
        glDeleteShader(frag_shader); // Don't leak the shader.
        release_string(vst);
        return -2;
    }
    release_string(vst);
    return frag_shader;
}

void lw_create_all_frag_shader(LWCONTEXT* pLwc) {
    for (int i = 0; i < LWFS_COUNT; i++) {
        pLwc->frag_shader[i] = lw_create_frag_shader(LWFS_DEFAULT + i, frag_shader_filename[i]);
    }
}

void lw_delete_all_frag_shader(LWCONTEXT* pLwc) {
    for (int i = 0; i < LWFS_COUNT; i++) {
        glDeleteShader(pLwc->frag_shader[i]);
    }
}
