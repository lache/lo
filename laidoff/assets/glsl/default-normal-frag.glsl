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

uniform sampler2D diffuse;
uniform float alpha_multiplier;
uniform vec3 overlay_color;
uniform float overlay_color_ratio;
uniform vec3 multiply_color;
varying vec3 normal;
varying vec2 uv;
// Outputs
FRAG_COLOR_OUTPUT_DECL

void main()
{
    vec4 t = TEX(diffuse, uv);
	vec3 l_dir = normalize(vec3(1.0,-1.0,1.0));
	vec3 light_col = vec3(0.3,0.3,0.3);
	float intensity = max(dot(normal, l_dir), 0.0);
    fragColor = (1.0 - overlay_color_ratio) * t + overlay_color_ratio * vec4(overlay_color, t.a);
	fragColor.rgb *= multiply_color;
	fragColor.rgb += light_col * intensity;
    fragColor.a *= alpha_multiplier;
}
