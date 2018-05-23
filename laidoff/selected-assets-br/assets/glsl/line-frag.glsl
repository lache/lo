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

// Outputs
FRAG_COLOR_OUTPUT_DECL

void main()
{
    fragColor = vec4(1.0,0.0,0.0,1.0);
}
