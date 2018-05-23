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
// Varying (inputs)
varying highp vec3      v_pColorOffset;
varying highp float		v_Growth;
varying highp float		v_Decay;
// Uniforms
uniform highp float     u_Time;
uniform highp vec3      u_eColorStart;
uniform highp vec3      u_eColorEnd;
uniform sampler2D       u_Texture;
uniform sampler2D       u_TextureAlpha;
// Outputs
FRAG_COLOR_OUTPUT_DECL

void main()
{
    highp vec4 texture1 = TEX(u_Texture, gl_PointCoord);
    highp vec4 textureAlpha = TEX(u_TextureAlpha, gl_PointCoord);

    // Color
    highp vec4 color = vec4(1.0);
    
    // If blast is growing
    if(u_Time < v_Growth)
    {
        // 1
        color.rgb = u_eColorStart;
    }
    
    // Else if blast is decaying
    else
    {
        highp float time = (u_Time - v_Growth) / v_Decay;
        
        // 2
        color.rgb = mix(u_eColorStart, u_eColorEnd, time);
    }
    color.rgb += v_pColorOffset;
    color.rgb = clamp(color.rgb, vec3(0.0), vec3(1.0));
    
    // Required OpenGL ES 2.0 outputs
    fragColor = texture1 * color;
    fragColor.a = textureAlpha.r;
}
