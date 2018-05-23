#ifdef GL_ES
#else
#define attribute in
#define varying out
#endif
// mediump causes z-fighting on iOS devices. highp required.
precision highp float;

// Uniforms
uniform mat4 uProjectionMatrix;
uniform float uK;
uniform float uTime;
// Attributes
attribute float aTheta;
attribute vec3 aShade;
// Outputs
varying vec3 vShade;

void main()
{
	float x = uTime * cos(uK*aTheta) * sin(aTheta);
	float y = uTime * cos(uK*aTheta) * cos(aTheta);
	
	gl_Position = uProjectionMatrix * vec4(x, y, 0.0, 1.0);
	gl_PointSize = 16.0;
	
	vShade = aShade;
}
