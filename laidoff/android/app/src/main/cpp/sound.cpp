#include "sound.h"
#include "lwlog.h"

void request_void_string_command(const char* command_name, const char* param1);

extern "C" int init_ext_sound_lib() {
    return 0;
}

void request_play_sound(const char* sound_type);

void request_stop_sound(const char* sound_type);

extern "C" void play_sound(LW_SOUND lws) {
    switch (lws) {
        case LWS_COLLAPSE:
            request_void_string_command("startCollapseSound", "dummy");
            break;
        case LWS_COLLISION:
            request_void_string_command("startCollisionSound", "dummy");
            break;
        case LWS_DAMAGE:
            request_void_string_command("startDamageSound", "dummy");
            break;
        case LWS_DASH1:
            request_void_string_command("startDash1Sound", "dummy");
            break;
        case LWS_DASH2:
            request_void_string_command("startDash2Sound", "dummy");
            break;
        case LWS_DEFEAT:
            request_void_string_command("startDefeatSound", "dummy");
            break;
        case LWS_INTROBGM:
            request_void_string_command("startIntroBgmSound", "dummy");
            break;
        case LWS_VICTORY:
            request_void_string_command("startVictorySound", "dummy");
            break;
        case LWS_SWOOSH:
            request_void_string_command("startSwooshSound", "dummy");
            break;
        case LWS_CLICK:
            request_void_string_command("startClickSound", "dummy");
            break;
        case LWS_READY:
            request_void_string_command("startReadySound", "dummy");
            break;
        case LWS_STEADY:
            request_void_string_command("startSteadySound", "dummy");
            break;
        case LWS_GO:
            request_void_string_command("startGoSound", "dummy");
            break;
        default:
            LOGEP("Unknown enum");
            break;
    }
}

extern "C" void stop_sound(LW_SOUND lws) {
}
