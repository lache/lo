#pragma once
#include <ode/ode.h>
#include "puckgamepacket.h"
typedef struct _LWPUCKGAMERECORDFRAMEGAMEOBJECT {
    dReal pos[3];
    dReal linvel[3];
    dReal quat[4];
    dReal angularvel[4];
} LWPUCKGAMERECORDFRAMEGAMEOBJECT;

typedef struct _LWPUCKGAMERECORDFRAMECONTROL {
    float dx;
    float dy;
    float dlen;
    int dash;
} LWPUCKGAMERECORDFRAMECONTROL;

typedef struct _LWPUCKGAMERECORDFRAME {
    int frame;
    LWPUCKGAMERECORDFRAMEGAMEOBJECT go[3]; // [3] --> [0]:puck, [1]:player, [2]:target
    LWPUCKGAMERECORDFRAMECONTROL control[2]; // [0]:player, [1]:target
    float dash_elapsed_since_last[2]; // [0]:player, [1]:target
    LWPSTATEBITFIELD bf;
} LWPUCKGAMERECORDFRAME;

typedef struct _LWPUCKGAMERECORD {
    LW_PUCK_GAME_MAP game_map;
    char nickname[LW_PUCK_GAME_MAX_PLAYER_COUNT_ON_TEAM][LW_NICKNAME_MAX_LEN];
    char target_nickname[LW_PUCK_GAME_MAX_PLAYER_COUNT_ON_TEAM][LW_NICKNAME_MAX_LEN];
    unsigned int id1[4];
    unsigned int id2[4];
    unsigned int id3[4];
    unsigned int id4[4];
    int player_score;
    int target_score;
    float dash_interval;
    int num_frame;
    LWPUCKGAMERECORDFRAME frames[125 /*frames per sec*/ * 60 /*total battle time*/];
} LWPUCKGAMERECORD;
