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
#define GLYPH_OUTLINE_MIX_THICKNESS 0.05

uniform sampler2D diffuse;
uniform float alpha_multiplier;
uniform vec3 overlay_color;
uniform float overlay_color_ratio;
uniform vec4 glyph_color;
uniform vec4 outline_color;
varying vec3 color;
varying vec2 uv;
// Outputs
FRAG_COLOR_OUTPUT_DECL

void main()
{
    vec4 t = TEX(diffuse, uv);
    fragColor.rgb = (glyph_color * t).rgb;
    fragColor.a = t.r;
}
