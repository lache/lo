#ifdef GL_ES
#define fragColor gl_FragColor
#define FRAG_COLOR_OUTPUT_DECL
#define TEX texture2D
#else
#define FRAG_COLOR_OUTPUT_DECL out vec4 fragColor;
#define varying in
#define TEX texture
#endif

precision mediump float;

uniform float alpha_multiplier;
uniform float gauge_ratio;
uniform vec3 full_color;
uniform vec3 empty_color;
varying vec2 uv;
// Outputs
FRAG_COLOR_OUTPUT_DECL

void main()
{
	fragColor = uv.y > gauge_ratio ? vec4(empty_color, 1) : vec4(full_color, 1);
	fragColor.a *= alpha_multiplier;
}
