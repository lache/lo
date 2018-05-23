#include "numcomp_puck_game.h"
#include "lwmacro.h"

void numcomp_puck_game_init(LWNUMCOMPPUCKGAME* numcomp) {
    numcomp_init_float_preset(&numcomp->f[LNFT_PUCK_REFLECT_SIZE],
                              sizeof(unsigned char) * 8,
                              1.0f,
                              2.0f);
    numcomp_init_float_preset(&numcomp->f[LNFT_PUCK_SPEED],
                              sizeof(unsigned short) * 8,
                              0.0f,
                              5.0f);
    numcomp_init_float_preset(&numcomp->f[LNFT_PUCK_MOVE_RAD],
                              sizeof(unsigned short) * 8,
                              (float)-M_PI,
                              (float)+M_PI);
    numcomp_init_vec3_preset(&numcomp->v[LNVT_POS],
                             11, -2.0f, +2.0f, // world x range
                             11, -2.0f, +2.0f, // world y range
                             10, +0.0f, +5.0f); //  jump range
    numcomp_init_quaternion_preset(&numcomp->q[LNQT_ROT]);
}
