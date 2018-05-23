#pragma once

#include "lwmacro.h"

#ifdef __cplusplus
extern "C" { ;
#endif

typedef enum _LW_SOUND {
    LWS_COLLAPSE,
    LWS_COLLISION,
    LWS_DAMAGE,
    LWS_DASH1,
    LWS_DASH2,
    LWS_DEFEAT,
    LWS_INTROBGM,
    LWS_VICTORY,
    LWS_SWOOSH,
    LWS_CLICK,
    LWS_READY,
    LWS_STEADY,
    LWS_GO,

    LWS_COUNT,
} LW_SOUND;

static const char* SOUND_FILE[] = {
    ASSETS_BASE_PATH "ogg" PATH_SEPARATOR "collapse.ogg",
    ASSETS_BASE_PATH "ogg" PATH_SEPARATOR "collision.ogg",
    ASSETS_BASE_PATH "ogg" PATH_SEPARATOR "damage.ogg",
    ASSETS_BASE_PATH "ogg" PATH_SEPARATOR "dash1.ogg",
    ASSETS_BASE_PATH "ogg" PATH_SEPARATOR "dash2.ogg",
    ASSETS_BASE_PATH "ogg" PATH_SEPARATOR "defeat.ogg",
    ASSETS_BASE_PATH "ogg" PATH_SEPARATOR "introbgm.ogg",
    ASSETS_BASE_PATH "ogg" PATH_SEPARATOR "victory.ogg",
    ASSETS_BASE_PATH "ogg" PATH_SEPARATOR "swoosh.ogg",
    ASSETS_BASE_PATH "ogg" PATH_SEPARATOR "click.ogg",
    ASSETS_BASE_PATH "ogg" PATH_SEPARATOR "ready.ogg",
    ASSETS_BASE_PATH "ogg" PATH_SEPARATOR "steady.ogg",
    ASSETS_BASE_PATH "ogg" PATH_SEPARATOR "go.ogg",
};

void play_sound(LW_SOUND lws);

#ifdef __cplusplus
}
#endif