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
uniform vec2 vS9Offset;
attribute vec3 vPos;
attribute vec3 vCol;
attribute vec2 vUv;
attribute vec2 vS9;
varying vec3 color;
varying vec2 uv;
varying float talpha;

void main()
{
    vec3 p = vPos;
    p.xy += vS9 * vS9Offset;
    gl_Position = MVP * vec4(p, 1.0);
    color = vCol;
    uv = vUvOffset + vUvScale * vUv;
	talpha = vPos.x * vPos.x + vPos.y * vPos.y;
	talpha = talpha * talpha;
}
