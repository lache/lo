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
uniform vec3 sphere_pos[3];
uniform vec3 sphere_col[3];
uniform float sphere_col_ratio[3];
uniform float sphere_speed[3];
uniform float sphere_move_rad[3];
uniform vec3 reflect_size;
varying vec3 color;
varying vec2 uv;
varying vec3 v;
// Outputs
FRAG_COLOR_OUTPUT_DECL

void main()
{
    vec4 t = TEX(diffuse, uv); // + vec4(color, 0.0);
    fragColor = (1.0 - overlay_color_ratio) * t + overlay_color_ratio * vec4(overlay_color, t.a);
    fragColor.a *= alpha_multiplier;
	
	float power = 15.0;
	float offset = 0.1;
	
	float A0 = sphere_move_rad[0];
	float cA0 = cos(A0);
	float sA0 = sin(A0);
	float d0_dx = v[0] - sphere_pos[0][0];
	float d0_dy = v[1] - sphere_pos[0][1];
	float d0_dz = v[2] - sphere_pos[0][2];
	float d0_density = 1.0 + sphere_speed[0] * sphere_speed[0];
	float d0_dx_r = d0_dx * cA0 + d0_dy * sA0;
	float d0_dy_r = d0_dx * sA0 - d0_dy * cA0;
	float d0 = sqrt(d0_density * d0_dx_r * d0_dx_r + d0_dy_r * d0_dy_r + d0_dz * d0_dz);
	
	float A1 = sphere_move_rad[1];
	float cA1 = cos(A1);
	float sA1 = sin(A1);
	float d1_dx = v[0] - sphere_pos[1][0];
	float d1_dy = v[1] - sphere_pos[1][1];
	float d1_dz = v[2] - sphere_pos[1][2];
	float d1_density = 1.0 + sphere_speed[1] * sphere_speed[1];
	float d1_dx_r = d1_dx * cA1 + d1_dy * sA1;
	float d1_dy_r = d1_dx * sA1 - d1_dy * cA1;
	float d1 = sqrt(d1_density * d1_dx_r * d1_dx_r + d1_dy_r * d1_dy_r + d1_dz * d1_dz);
	
	float A2 = sphere_move_rad[2];
	float cA2 = cos(A2);
	float sA2 = sin(A2);
	float d2_dx = v[0] - sphere_pos[2][0];
	float d2_dy = v[1] - sphere_pos[2][1];
	float d2_dz = v[2] - sphere_pos[2][2];
	float d2_density = 1.0 + sphere_speed[2] * sphere_speed[2];
	float d2_dx_r = d2_dx * cA2 + d2_dy * sA2;
	float d2_dy_r = d2_dx * sA2 - d2_dy * cA2;
	float d2 = sqrt(d2_density * d2_dx_r * d2_dx_r + d2_dy_r * d2_dy_r + d2_dz * d2_dz);
	
	float w = 0.5;
	
	float r0 = 1.0 / (1.0 + exp(power * (w*d0 - offset))) * reflect_size[0];
	float r1 = 1.0 / (1.0 + exp(power * (w*d1 - offset))) * reflect_size[1];
	float r2 = 1.0 / (1.0 + exp(power * (w*d2 - offset))) * reflect_size[2];
	
	fragColor.rgb += r0 * sphere_col[0] * sphere_col_ratio[0];
	fragColor.rgb += r1 * sphere_col[1] * sphere_col_ratio[1];
	fragColor.rgb += r2 * sphere_col[2] * sphere_col_ratio[2];
}
