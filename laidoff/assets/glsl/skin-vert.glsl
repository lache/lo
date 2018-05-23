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
uniform mat4 bone[40];

attribute vec3 vPos;
attribute vec3 vCol;
attribute vec2 vUv;
attribute vec4 vBw;
attribute vec4 vBm;
//varying vec3 color;
varying vec2 uv;
varying vec3 v;

void main()
{
    vec4 p = vec4(vPos, 1.0);
	int bmx = int(vBm.x);
	int bmy = int(vBm.y);
	int bmz = int(vBm.z);
	int bmw = int(vBm.w);
    
    v = vec3(p);

	float weight_sum = vBw.x + vBw.y + vBw.z + vBw.w;

	vec4 ps = (1.0 - weight_sum) * p
			+ vBw.x * bone[bmx] * p
			+ vBw.y * bone[bmy] * p
			+ vBw.z * bone[bmz] * p
			+ vBw.w * bone[bmw] * p;
	
    gl_Position = MVP * ps;
	
    //color = vCol;
    uv = vUvOffset + vUvScale * vUv;
}
