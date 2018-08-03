#ifdef GL_ES
#define fragColor gl_FragColor
#define FRAG_COLOR_OUTPUT_DECL
#else
#define FRAG_COLOR_OUTPUT_DECL out vec4 fragColor;
#define varying in
#endif

precision lowp float;
#define MAX_ITER 2
uniform float time;
uniform vec2 resolution;
// Outputs
FRAG_COLOR_OUTPUT_DECL

vec2 R = resolution;
vec2 Offset;
vec2 Scale=vec2(0.002,0.002);
float Saturation = 0.8; // 0 - 1;


vec3 lungth(vec2 x,vec3 c){
       return vec3(length(x+c.r),length(x+c.g),length(c.b));
}

void main(void) {
	vec3 light_color = vec3(2,2,2);
	
	float t = time * 1.0;
	vec2 position = ( gl_FragCoord.xy * 2.0 -  resolution.xy) / resolution.x;

	// 256 angle steps
	float angle = atan(position.y, position.x) / (3.14159265359);
	//angle -= floor(angle);
	float rad = length(position);
	
	vec3 color = vec3(0.0);

	float angleFract = fract(angle*256.);
	float angleRnd = floor(angle*256.)+100.;
	float angleRnd1 = fract(angleRnd*fract(angleRnd*.7235)*45.1);
	float angleRnd2 = fract(angleRnd*fract(angleRnd*.82657)*13.724);
	float t2 = t + angleRnd1*100.0;
	float radDist = sqrt(angleRnd2);
	
	float adist = radDist / rad * 0.1;
	float dist = (t2*.1+adist);
	dist = abs(fract(dist) - 1.0);
	color +=  (1.0 / dist) * cos(sin(t)) * adist / radDist / 30.0;  // cos(sin(t)) make endless.
	//color.xy = position.xy;
	fragColor = vec4(color.r, color.g, color.b,1);
}

void main5(void) {
		vec2 position = ( gl_FragCoord.xy / resolution.xy ) + 2.0 / 4.0;

	float color = 0.0;
	color += sin( position.x * cos( time / 15.0 ) * 80.0 ) + cos( position.y * cos( time / 15.0 ) * 10.0 );
	color += sin( position.y * sin( time / 10.0 ) * 40.0 ) + cos( position.x * sin( time / 25.0 ) * 40.0 );
	color += sin( position.x * sin( time / 5.0 ) * 10.0 ) + sin( position.y * sin( time / 35.0 ) * 80.0 );
	color *= sin( time / 10.0 ) * 0.5;

	fragColor = vec4( vec3( color, color * 0.5, sin( color + time / 3.0 ) * 0.75 ), 1.0 );
}

void main4(void) {
		vec2 position = (gl_FragCoord.xy - resolution * 0.5) / resolution.yy;
	float th = atan(position.y, position.x) / (0.2 * 3.1415926);
	float dd = length(position) + 0.005 + 0.1*sin(time);
	float d = 0.5 / dd + time;
	
    	vec2 x = gl_FragCoord.xy;
    	vec3 c2=vec3(0,0,0);
   	x=x*Scale*R/R.x;
    	x+sin(x.yx*sqrt(vec2(1,9)))/1.;
    	c2=lungth(sin(x*sqrt(vec2(3,43))),vec3(5,6,7)*Saturation * d);
	x+=sin(x.yx*sqrt(vec2(73,5)))/5.;
    	c2=2.*lungth(sin(time+x*sqrt(vec2(33.,23.))),c2/9.);
    	x+=sin(x.yx*sqrt(vec2(93,7)))/3.;
    	c2=lungth(sin(x*sqrt(vec2(3.,1.))),c2/2.0);
    	c2=.5+.5*sin(c2*8.);
	
	vec3 uv = vec3(th + d, th - d, th + sin(d) * 0.45);
	float a = 0.5 + cos(uv.x * 3.1415926 * 2.0) * 0.5;
	float b = 0.5 + cos(uv.y * 3.1415926 * 2.0) * 0.5;
	float c = 0.5 + cos(uv.z * 3.1415926 * 6.0) * 0.5;
	vec3 color = 	mix(vec3(0.1, 0.5, 0.5), 	vec3(0.1, 0.1, 0.2),  pow(a, 0.2)) * 3.;
	color += 	mix(vec3(0.8, 0.2, 1.0), 	vec3(0.1, 0.1, 0.2),  pow(b, 0.1)) * 0.75;
	color += 	mix(c2, 			vec3(0.1, 0.2, 0.2),  pow(c, 0.1)) * 0.75;

	fragColor = vec4( (color * dd), 1.5);
}

void main3(void) {
	            vec2 v_texCoord = gl_FragCoord.xy / resolution;

            vec2 p =  v_texCoord * 8.0 - vec2(20.0);
            vec2 i = p;
            float c = 1.0;
            float inten = .05;

            for (int n = 0; n < MAX_ITER; n++)
            {
                float t = time * (2.0 - (3.0 / float(n+1)));

                i = p + vec2(cos(t - i.x) + sin(t + i.y),
                sin(t - i.y) + cos(t + i.x));
		    
                c += 1.0/length(vec2(p.x / (sin(i.x+t)/inten),
                p.y / (cos(i.y+t)/inten)));
            }

            c /= float(MAX_ITER);
            c = 1.5 - sqrt(c);

            vec4 texColor = vec4(0.050, 0.05, 0.2, 1.);

            texColor.rgb *= (1.0/ (1.0 - (c + 0.05)));

            fragColor = texColor;
}

void main2( void ) {
	
	vec2 position = (gl_FragCoord.xy - resolution * 0.9) / resolution.yy;
	float th = atan(position.y, position.x) / (1.0 * 3.1415926);
	float dd = length(position) + 0.005;
	float d = 0.5 / dd + time;
	
    	vec2 x = gl_FragCoord.xy;
    	vec3 c2=vec3(0,0,0);
   	x=x*Scale*R/R.x;
    	x+sin(x.yx*sqrt(vec2(1,9)))/1.;
    	c2=lungth(sin(x*sqrt(vec2(3,43))),vec3(5,6,7)*Saturation * d);
	x+=sin(x.yx*sqrt(vec2(73,5)))/5.;
    	c2=2.*lungth(sin(time+x*sqrt(vec2(33.,23.))),c2/9.);
    	x+=sin(x.yx*sqrt(vec2(93,7)))/3.;
    	c2=lungth(sin(x*sqrt(vec2(3.,1.))),c2/2.0);
    	c2=.5+.5*sin(c2*8.);
	
	vec3 uv = vec3(th + d, th - d, th + sin(d) * 0.45);
	float a = 0.5 + cos(uv.x * 3.1415926 * 2.0) * 0.5;
	float b = 0.5 + cos(uv.y * 3.1415926 * 2.0) * 0.5;
	float c = 0.5 + cos(uv.z * 3.1415926 * 6.0) * 0.5;
	vec3 color = 	mix(vec3(0.1, 0.5, 0.5), 	vec3(0.1, 0.1, 0.2),  pow(a, 0.2)) * 3.;
	color += 	mix(vec3(0.8, 0.2, 1.0), 	vec3(0.1, 0.1, 0.2),  pow(b, 0.1)) * 0.75;
	color += 	mix(c2, 			vec3(0.1, 0.2, 0.2),  pow(c, 0.1)) * 0.75;

	fragColor = vec4( (color * dd) * 0.15, 1.0);
}
