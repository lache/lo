#pragma once 

typedef struct _LWVERTEX {
	float x, y, z;
	float r, g, b;
	float u, v;
	float s9x, s9y;
} LWVERTEX;

static const LWVERTEX full_square_vertices[] =
{
    //x     y     z  r  g  b  u  v  s  s
	{ -1.f, -1.f, 0, 0, 0, 0, 0, 1, 0, 0 },
	{ +1.f, -1.f, 0, 0, 0, 0, 1, 1, 0, 0 },
	{ +1.f, +1.f, 0, 0, 0, 0, 1, 0, 0, 0 },
	{ +1.f, +1.f, 0, 0, 0, 0, 1, 0, 0, 0 },
	{ -1.f, +1.f, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ -1.f, -1.f, 0, 0, 0, 0, 0, 1, 0, 0 },
};

#define VERTEX_COUNT_PER_ARRAY (6)

typedef struct _LWSKINVERTEX{
	float x, y, z;
	float r, g, b;
	float u, v;
	float bw0, bw1, bw2, bw3;
	float bm0, bm1, bm2, bm3;
} LWSKINVERTEX;

#define FAN_SECTOR_COUNT_PER_ARRAY (72/2) // 5*2 degrees interval
#define FAN_VERTEX_COUNT_PER_ARRAY (1 + FAN_SECTOR_COUNT_PER_ARRAY + 1) // 1 for a center vertex, 1 for the end vertex

typedef struct _LWFANVERTEX {
	float r;
	float theta;
	float index;
} LWFANVERTEX;

typedef struct _LWCOLORVERTEX {
    float x, y, z;
    float nx, ny, nz;
    float r, g, b;
} LWCOLORVERTEX;

typedef struct _LWLINEVERTEX {
    float x, y;
} LWLINEVERTEX;

typedef struct _LWMORPHVERTEX {
    float x0, y0, z0;
    float x1, y1, z1;
    float u, v;
} LWMORPHVERTEX;
