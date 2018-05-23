#ifdef GL_ES
#define fragColor gl_FragColor
#define FRAG_COLOR_OUTPUT_DECL
#else
#define FRAG_COLOR_OUTPUT_DECL out vec4 fragColor;
#define varying in
#endif

precision highp float;
// Outputs
FRAG_COLOR_OUTPUT_DECL

void main()
{
    fragColor = vec4(1.0, 1.0, 1.0, 0.5);
}
