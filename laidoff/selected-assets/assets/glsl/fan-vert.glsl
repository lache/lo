#ifdef GL_ES
#else
#define attribute in
#define varying out
#endif
// mediump causes z-fighting on iOS devices. highp required.
precision highp float;

uniform mat4 MVP;
uniform float rscale[1 + 72 + 1];
uniform float thetascale;
attribute vec3 vPos;

void main()
{
	// vPos: R, Theta, Index
	
	int vPosUz = int(vPos.z);
	
	vec2 p;
	p.x = vPos.x * cos(vPos.y * thetascale) * rscale[vPosUz];
	p.y = vPos.x * sin(vPos.y * thetascale) * rscale[vPosUz];
	
    gl_Position = MVP * vec4(p, 0.0, 1.0);
}
