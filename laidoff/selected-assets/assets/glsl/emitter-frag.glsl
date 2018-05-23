#ifdef GL_ES
#define fragColor gl_FragColor
#define FRAG_COLOR_OUTPUT_DECL
#define TEX texture2D
#else
#define FRAG_COLOR_OUTPUT_DECL out vec4 fragColor;
#define varying in
#define TEX texture
#endif

precision highp float;
// Uniforms
uniform vec3 uColor;
uniform sampler2D uTexture;
// Inputs from vertex shader
varying vec3 vShade;
// Outputs
FRAG_COLOR_OUTPUT_DECL


void main()
{
	vec4 texture = TEX(uTexture, gl_PointCoord);
	vec4 color = vec4(uColor + vShade, 1.0);
	color.rgb = clamp(color.rgb, vec3(0.0), vec3(1.0));
	fragColor = texture * color;
}
