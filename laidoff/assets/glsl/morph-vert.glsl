#ifdef GL_ES
#else
#define attribute in
#define varying out
#endif
// mediump causes z-fighting on iOS devices. highp required.
precision highp float;

uniform mat4 MVP;
uniform vec2 vUvOffset;
uniform vec2 vUvScale;
uniform float morph_weight;
attribute vec3 vPos;
attribute vec3 vPos2;
attribute vec2 vUv;
varying vec3 color;
varying vec2 uv;

void main()
{
    vec3 p = (1.0 - morph_weight) * vPos + morph_weight * vPos2;
    gl_Position = MVP * vec4(p, 1.0);
    color = vec3(1.0, 1.0, 1.0);
    uv = vUvOffset + vUvScale * vUv;
}
