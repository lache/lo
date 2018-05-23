#ifdef GL_ES
#else
#define attribute in
#define varying out
#endif
// mediump causes z-fighting on iOS devices. highp required.
precision highp float;

uniform mat4 MVP;
attribute vec2 vPos;

void main()
{
    gl_Position = MVP * vec4(vPos, 0.0, 1.0);
}
