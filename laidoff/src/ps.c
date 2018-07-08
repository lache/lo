#include "ps.h"
#include "lwmacro.h"
#include "lwcontext.h"
#include "lwgl.h"
#include "pcg_basic.h"
#include <stdlib.h>
#include <string.h>

#define NUM_PS_INSTANCE (16)

typedef struct _LWPSINST {
    int valid;
    LWEMITTER2OBJECT emit_object;
} LWPSINST;

typedef struct _LWPS {
    LWPSINST inst[NUM_PS_INSTANCE];
} LWPS;

float oDecay = 0.25f;       // Time

// return [0, 1) double
static double random_double(LWPSCONTEXT* c) {
    return ldexp(pcg32_random_r(&c->rng), -32);
}

static float random_float_between(LWPSCONTEXT* c, float min, float max) {
    float range = max - min;
    return (float)(min + random_double(c) * range);
}

void load_emitter2(LWCONTEXT* pLwc, LWPSCONTEXT* c) {
    // 3
    // Offset bounds
    float oRadius1 = 0.1f; //pLwc->viewport_width / 640.0f * 0.10f;      // 0.0 = circle; 1.0 = ring
    float oRadius2 = 1.0f;
    float oVelocity = 1.50f;    // Speed

    float size_offset_range = 10.0f / 2;
    float oSize = LWMIN(size_offset_range, pLwc->viewport_width / 1920.0f * size_offset_range);        // Pixels
    float oColor = 0.25f;       // 0.5 = 50% shade offset

    // 4
    // Load Particles
    int angle = 2;
    for (int i = 0; i < NUM_PARTICLES2; i++) {
        // Assign a unique ID to each particle, between 0 and 360 (in radians)
        c->emitter2.eParticles[i].pId = (float)LWDEG2RAD(((float)(-angle / 2 + i % angle) / (float)NUM_PARTICLES2)*360.0f);

        // Assign random offsets within bounds
        c->emitter2.eParticles[i].pRadiusOffset = random_float_between(c, oRadius1, oRadius2);
        c->emitter2.eParticles[i].pVelocityOffset = random_float_between(c, -oVelocity, oVelocity);
        c->emitter2.eParticles[i].pDecayOffset = random_float_between(c, -oDecay, oDecay);
        c->emitter2.eParticles[i].pSizeOffset = random_float_between(c, -oSize, oSize);
        float r = random_float_between(c, -oColor, oColor);
        float g = random_float_between(c, -oColor, oColor);
        float b = random_float_between(c, -oColor, oColor);
        c->emitter2.eParticles[i].pColorOffset[0] = r;
        c->emitter2.eParticles[i].pColorOffset[1] = g;
        c->emitter2.eParticles[i].pColorOffset[2] = b;
    }

    // 5
    // Load Properties
    c->emitter2.eRadius = 6.0f;                                     // Blast radius
    c->emitter2.eVelocity = 3.00f;                                   // Explosion velocity
    c->emitter2.eDecay = 2.00f;                                      // Explosion decay
    c->emitter2.eSizeStart = LWMIN(50.0f / 2, pLwc->viewport_width / 1920.0f * 50.0f);        // Pixels
    c->emitter2.eSizeEnd = LWMIN(16.0f / 2, pLwc->viewport_width / 1920.0f * 16.0f);        // Pixels
    c->emitter2.eColorStart[0] = 1.0f;
    c->emitter2.eColorStart[1] = 0.5f;
    c->emitter2.eColorStart[2] = 0.0f;
    c->emitter2.eColorEnd[0] = 0.25f;
    c->emitter2.eColorEnd[1] = 0.0f;
    c->emitter2.eColorEnd[2] = 0.0f;
    // Set global factors
    float growth = c->emitter2.eRadius / c->emitter2.eVelocity;       // Growth time
    c->emitter2_object.life = growth + c->emitter2.eDecay + oDecay;                    // Simulation lifetime
                                          // Drag (air resistance)
    c->emitter2_object.gravity[0] = 0;
    c->emitter2_object.gravity[1] = 0;// -9.81f * (1.0f / drag);              // 7

    glGenBuffers(1, &pLwc->particle_buffer2);
    glBindBuffer(GL_ARRAY_BUFFER, pLwc->particle_buffer2);
    glBufferData(GL_ARRAY_BUFFER, sizeof(c->emitter2.eParticles), c->emitter2.eParticles, GL_STATIC_DRAW);
}

void ps_load_particles(LWCONTEXT* pLwc, LWPSCONTEXT* c) {
    pcg32_srandom_r(&c->rng, 0x0DEEC2CBADF00D77, 0x15881588CA11DAC1);
    //pcg32_srandom_r(&rng, 3, 56);

    for (int i = 0; i < NUM_PARTICLES; i++) {
        c->emitter.particles[i].theta = (float)LWDEG2RAD(i);
        c->emitter.particles[i].shade[0] = random_float_between(c, -0.25f, 0.25f);
        c->emitter.particles[i].shade[1] = random_float_between(c, -0.25f, 0.25f);
        c->emitter.particles[i].shade[2] = random_float_between(c, -0.25f, 0.25f);
    }
    glGenBuffers(1, &pLwc->particle_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, pLwc->particle_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(c->emitter.particles), c->emitter.particles, GL_STATIC_DRAW);

    load_emitter2(pLwc, c);
}

void ps_load_emitter(LWCONTEXT* pLwc, LWPSCONTEXT* c) {
    c->emitter.k = 4.0f;
    c->emitter.color[0] = 0.76f;
    c->emitter.color[1] = 0.12f;
    c->emitter.color[2] = 0.34f;

    c->time_current = 0.0f;
    c->time_max = 3.0f;
    c->time_direction = 1;

    c->emitter2_object.gravity[0] = 0;
    c->emitter2_object.gravity[1] = 0;
    c->emitter2_object.life = 0;
    c->emitter2_object.time = 0;
}

void ps_test_update(LWCONTEXT* pLwc, LWPSCONTEXT* c) {
    if (c->time_current > c->time_max) {
        c->time_direction = -1;
    }
    else if (c->time_current < 0.0f) {
        c->time_direction = 1;
    }
    const float time_elapsed = (float)lwcontext_delta_time(pLwc);
    c->time_current += (float)(c->time_direction * time_elapsed);

    c->emitter2_object.time += time_elapsed;
    if (c->emitter2_object.time > c->emitter2_object.life) {
        c->emitter2_object.time = 0.0f;
    }
}

void ps_update(LWPS* ps, double delta_time) {
    for (int i = 0; i < NUM_PS_INSTANCE; i++) {
        if (!ps->inst[i].valid) {
            continue;
        }
        ps->inst[i].emit_object.time += (float)delta_time;
        if (ps->inst[i].emit_object.time > ps->inst[i].emit_object.life) {
            ps->inst[i].valid = 0;
        }
    }
}

void* ps_new() {
    return calloc(1, sizeof(LWPS));
}

void ps_destroy(LWPS** ps) {
    free(*ps);
    *ps = 0;
}

void ps_play_new(LWPSCONTEXT* c, LWPS* ps) {
    vec3 zero = { 0, 0, 0 };
    ps_play_new_pos(c, ps, zero);
}

void ps_play_new_pos(LWPSCONTEXT* c, LWPS* ps, const vec3 pos) {
    for (int i = 0; i < NUM_PS_INSTANCE; i++) {
        if (ps->inst[i].valid) {
            continue;
        }
        ps->inst[i].valid = 1;

        float growth = c->emitter2.eRadius / c->emitter2.eVelocity;
        ps->inst[i].emit_object.life = growth + c->emitter2.eDecay + oDecay;

        ps->inst[i].emit_object.gravity[0] = 0;
        ps->inst[i].emit_object.gravity[1] = 0;// -9.81f * (1.0f / drag);
        ps->inst[i].emit_object.time = 0;
        memcpy(ps->inst[i].emit_object.pos, pos, sizeof(vec3));
        break;
    }
}

static LWEMITTER2OBJECT* s_ps_emit_object_find_first_valid(LWPS* ps, int start_index) {
    if (!ps) {
        return 0;
    }
    for (int i = start_index; i < NUM_PS_INSTANCE; i++) {
        if (ps->inst[i].valid) {
            return &ps->inst[i].emit_object;
        }
    }
    return 0;
}

LWEMITTER2OBJECT* ps_emit_object_begin(LWPS* ps) {
    return s_ps_emit_object_find_first_valid(ps, 0);
}

LWEMITTER2OBJECT* ps_emit_object_next(LWPS* ps, LWEMITTER2OBJECT* cursor) {
    int start_index = ((char*)cursor - (char*)&ps->inst[0].emit_object) / sizeof(LWPSINST);
    return s_ps_emit_object_find_first_valid(ps, start_index + 1);
}

void* ps_new_context() {
    return calloc(1, sizeof(LWPSCONTEXT));
}

void ps_destroy_context(void** c) {
    free(*c);
    *c = 0;
}
