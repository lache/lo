#pragma once
#include "linmath.h"
#include "lwmacro.h"

typedef struct _LWKEYFRAME LWKEYFRAME;
typedef struct _LWCONTEXT LWCONTEXT;
typedef void(*custom_render_proc)(const LWCONTEXT*, float, float, float);
typedef void(*anim_finalized_proc)(LWCONTEXT*);

typedef struct _LWANIM {
	int valid;
	int las;
	float elapsed;
	float fps;
	const LWKEYFRAME* animdata;
	int animdata_length;
	//int current_animdata_index;
	mat4x4 mvp;
	float x;
	float y;
	float alpha;
	custom_render_proc custom_render_proc_callback;
	anim_finalized_proc anim_finalized_proc_callback;
	int finalized;
} LWANIM;

// Should match KeyframePointType enum at export_action.py
typedef enum _LW_ANIM_CURVE_TYPE {
	LACT_NONE = 0,
	LACT_KEYFRAME = 1,
	LACT_BEZIER = 2,
	LACT_AUTO = 3,
	LACT_AUTO_CLAMPED = 4,
	LACT_LINEAR = 5,
	LACT_LOCATION = 6,
	LACT_ROTATION_QUATERNION = 7,
	LACT_SCALE = 8,
} LW_ANIM_CURVE_TYPE;

typedef struct _LWANIMKEY {
	float co[2]; // coordinates
	float hl[2]; // handle left coordinates
	float hr[2]; // handle right coordinates
	LW_ANIM_CURVE_TYPE type : 8;
	LW_ANIM_CURVE_TYPE interpolation : 8;
	LW_ANIM_CURVE_TYPE easing : 8;
	LW_ANIM_CURVE_TYPE hlt : 8; // handle left type
	LW_ANIM_CURVE_TYPE hrt : 8; // handle right type
	LW_ANIM_CURVE_TYPE padding_0 : 8;
	LW_ANIM_CURVE_TYPE padding_1 : 8;
	LW_ANIM_CURVE_TYPE padding_2 : 8;
} LWANIMKEY;

typedef struct _LWANIMCURVE {
	int bone_index;
	LW_ANIM_CURVE_TYPE anim_curve_type;
	int anim_curve_index;
	int key_offset;
	int key_num;
} LWANIMCURVE;

typedef struct _LWANIMMARKER {
	char name[32];
	int f;
} LWANIMMARKER;

#define MAX_ANIM_CURVE (32 * 4)

typedef struct _LWANIMACTION {
	float fps;
	float last_key_f;
	int anim_curve_num;
	LWANIMCURVE* anim_curve;
	int anim_key_num;
	LWANIMKEY* anim_key;
	int anim_marker_num;
	LWANIMMARKER* anim_marker;
	char* d;
} LWANIMACTION;

typedef enum _LW_ACTION {
	LWAC_TRIANGLEACTION,
	LWAC_TREEPLANEACTION,
	LWAC_HUMANACTION_WALKPOLISH,
	LWAC_HUMANACTION_IDLE,
	LWAC_HUMANACTION_ATTACK,
	LWAC_HUMANACTION_STAND_AIM,
	LWAC_HUMANACTION_STAND_UNAIM,
	LWAC_HUMANACTION_STAND_FIRE,
	LWAC_HUMANACTION_DEATH,
	LWAC_DETACHPLANEACTION,
	LWAC_DETACHPLANEACTION_CHILDTRANS,
	LWAC_RECOIL,
	LWAC_TURRET_RECOIL,
	LWAC_CROSSBOW_FIRE,
	LWAC_CATAPULT_FIRE,
	LWAC_PYRO_IDLE,

	LWAC_COUNT,
} LW_ACTION;

extern const char* action_filename[LWAC_COUNT];

void load_action(const char* filename, LWANIMACTION* action);
void unload_action(LWANIMACTION* action);
int get_curve_value(const LWANIMKEY* key, int key_len, float t, float* v);
float lwanimaction_animtime_to_f(const LWANIMACTION* action, float animtime);
