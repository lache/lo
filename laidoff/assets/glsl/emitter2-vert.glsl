#ifdef GL_ES
#else
#define attribute in
#define varying out
#endif
precision highp float;
// Uniforms
uniform mat4        u_ProjectionViewMatrix;
uniform mat4        u_ModelMatrix;
uniform vec2        u_Gravity;
uniform float       u_Time;
uniform float       u_eRadius;
uniform float       u_eVelocity;
uniform float       u_eDecay;
uniform float       u_eSizeStart;
uniform float       u_eSizeEnd;
uniform float       u_eScreenWidth;
// Attributes (inputs)
attribute float     a_pID;
attribute float     a_pRadiusOffset;
attribute float     a_pVelocityOffset;
attribute float     a_pDecayOffset;
attribute float     a_pSizeOffset;
attribute vec3      a_pColorOffset;
// Varying (outputs)
varying vec3        v_pColorOffset;
varying float		v_Growth;
varying float		v_Decay;

void main(void)
{
    // Convert polar angle to cartesian coordinates and calculate radius
    float x = cos(a_pID);
    float y = sin(a_pID);
    float r = u_eRadius * a_pRadiusOffset;
    
    // Lifetime
    float growth = r / (u_eVelocity + a_pVelocityOffset);
    float decay = u_eDecay + a_pDecayOffset;
    
    // Size
    float s = 1.0; // dummy initial value (always assigned again)
    
	float time = 1.0;
	
    // If blast is growing
    if(u_Time < growth)
    {
        time = u_Time / growth;
        x = x * r * time;
        y = y * r * time;
        
        // 1
        s = u_eSizeStart;
    }
    
    // Else if blast is decaying
    else
    {
        time = (u_Time - growth) / decay;
        x = (x * r) + (u_Gravity.x * time);
        y = (y * r) + (u_Gravity.y * time);
        
        // 2
        s = mix(u_eSizeStart, u_eSizeEnd, time);
    }
    
    // Required OpenGL ES 2.0 outputs
    gl_Position = u_ProjectionViewMatrix * u_ModelMatrix * vec4(x, y, 0.0, 1.0);
    
    // 3
    gl_PointSize = min(60.0, max(0.0, (s + a_pSizeOffset)));
    
    // Fragment Shader outputs
    v_pColorOffset = a_pColorOffset;
    v_Growth = growth;
    v_Decay = decay;
}
