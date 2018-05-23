#ifdef GL_ES
#else
#define attribute in
#define varying out
#endif

// mediump causes z-fighting on iOS devices. highp required.
precision highp float;
uniform mat4 MVP;
uniform mat4 M;
attribute vec3 vPos;
attribute vec3 vNor;
attribute vec3 vCol;
varying vec4 normal;
varying vec3 color;

void main()
{
    vec3 p = vPos;
    gl_Position = MVP * vec4(p, 1.0);
    normal = M * vec4(vNor, 0.0);
	
	vec3 ambient = vec3(0.1,0.1,0.1);
	vec4 l_dir = normalize(vec4(0.5,-0.4,1.0,0.0));
	float intensity = max(dot(normal, l_dir), 0.0);
	color = ambient + vCol * intensity;
}
