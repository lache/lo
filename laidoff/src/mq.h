#pragma once

#define LW_KVMSG_KEY_MAX (255) // SHOULD MATCH KVMSG_KEY_MAX

void* init_mq(const char* addr, void* sm);
void deinit_mq(void* _mq);
void mq_poll(void* pLwc, void* sm, void* _mq, void* field);
void* mq_rmsg_reader(void* _mq);
void* mq_rmsg_writer(void* _mq);
void mq_shutdown();

typedef struct _LWANIMACTION LWANIMACTION;
typedef struct _LWMESSAGEQUEUE LWMESSAGEQUEUE;

// ALIGNMENT matters!
typedef struct _LWMQMSG {
	double t;				// Time
	float x;				// Current position X
	float y;				// Current position Y
	float z;				// Current position Z
	float dx;				// Last moved delta X
	float dy;				// Last moved delta y
	//int moving;				// 1 if moving, 0 if stopped
	//int attacking;			// 1 if attacking, 0 if stopped
	int stop;				// 1 if movement stopped, 0 if not
	int action;				// LW_ACTION enum
} LWMQMSG;

// ALIGNMENT matters!
typedef struct _LWFIREMSG {
	int type;
	float pos[3];
	float vel[3];
	int bullet_id;
	char owner_key[LW_KVMSG_KEY_MAX + 1];
} LWFIREMSG;

// ALIGNMENT matters!
typedef struct _LWHITMSG {
	int type;
	char target_key[LW_KVMSG_KEY_MAX + 1];
} LWHITMSG;

// ALIGNMENT matters!
typedef struct _LWDESPAWNBULLETMSG {
	int type;
	int bullet_id;
	char owner_key[LW_KVMSG_KEY_MAX + 1];
} LWDESPAWNBULLETMSG;

// ALIGNMENT matters!
typedef struct _LWACTIONMSG {
	int type;
	int action;
} LWACTIONMSG;

typedef struct _LWPOSSYNCMSG {
	char key[LW_KVMSG_KEY_MAX + 1];		// User key
	float x;							// Last position X (extrapolated)
	float y;							// Last position Y (extrapolated)
	float z;							// Last position Z (extrapolated)
	float a;							// Last orientation (extrapolated)
	void* extrapolator;					// Extrapolator for a remote entity
	//int moving;						// 1 if moving, 0 if stopped
	//int attacking;					// 1 if attacking, 0 if stopped
	int action;							// LW_ACTION enum
	const LWANIMACTION* anim_action;	// Last anim action
	void* field;						// Reference to the current field instance
	int geom_index;						// User geom index
	float flash;						// Hit flash opacity (0~1)
} LWPOSSYNCMSG;

const LWMQMSG* mq_sync_first(void* _mq);
const char* mq_sync_cursor(void* _mq);
const LWMQMSG* mq_sync_next(void* _mq);

LWPOSSYNCMSG* mq_possync_first(void* _mq);
const char* mq_possync_cursor(void* _mq);
LWPOSSYNCMSG* mq_possync_next(void* _mq);

const char* mq_uuid_str(void* _mq);
const char* mq_subtree(void* _mq);

void mq_publish_now(void* pLwc, void* _mq, int stop);

double mq_mono_clock();
double mq_sync_mono_clock(void* _mq);
int mq_cursor_player(void* _mq, const char* cursor);
void mq_interrupt();
void mq_send_fire(LWMESSAGEQUEUE* mq, const float* pos, const float* vel);
void mq_send_action(LWMESSAGEQUEUE* mq, int action);
void mq_lock_mutex(void* _mq);
void mq_unlock_mutex(void* _mq);
void mq_send_hit(void* _mq, LWPOSSYNCMSG* possyncmsg);
void mq_send_despawn_bullet(void* _mq, int bullet_id);
int mq_bullet_counter(LWMESSAGEQUEUE* mq);
