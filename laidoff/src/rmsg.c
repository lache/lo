#include "rmsg.h"
#include "mq.h"
#include "zmq.h"
#include "lwcontext.h"
#include <string.h>

static void s_send_and_close_rmsg(LWCONTEXT* pLwc, zmq_msg_t* rmsg) {
	zmq_msg_send(rmsg, mq_rmsg_writer(lwcontext_mq(pLwc)), 0);
	lwcontext_inc_rmsg_send(pLwc);
	zmq_msg_close(rmsg);
}

void rmsg_spawn(LWCONTEXT* pLwc, int key, int objtype, float x, float y, float z, float angle) {
	zmq_msg_t rmsg;
	zmq_msg_init_size(&rmsg, sizeof(LWFIELDRENDERCOMMAND));
	LWFIELDRENDERCOMMAND* cmd = zmq_msg_data(&rmsg);
	memset(cmd, 0, sizeof(LWFIELDRENDERCOMMAND));
	cmd->cmdtype = LRCT_SPAWN;
	cmd->key = key;
	cmd->objtype = objtype;
	cmd->pos[0] = x;
	cmd->pos[1] = y;
	cmd->pos[2] = z;
	cmd->scale[0] = 1;
	cmd->scale[1] = 1;
	cmd->scale[2] = 1;
	cmd->angle = angle;
	cmd->actionid = LWAC_RECOIL;
	s_send_and_close_rmsg(pLwc, &rmsg);
}

void rmsg_despawn(LWCONTEXT* pLwc, int key) {
	zmq_msg_t rmsg;
	zmq_msg_init_size(&rmsg, sizeof(LWFIELDRENDERCOMMAND));
	LWFIELDRENDERCOMMAND* cmd = zmq_msg_data(&rmsg);
	cmd->cmdtype = LRCT_DESPAWN;
	cmd->key = key;
	s_send_and_close_rmsg(pLwc, &rmsg);
}

void rmsg_pos(LWCONTEXT* pLwc, int key, float x, float y, float z) {
	zmq_msg_t rmsg;
	zmq_msg_init_size(&rmsg, sizeof(LWFIELDRENDERCOMMAND));
	LWFIELDRENDERCOMMAND* cmd = zmq_msg_data(&rmsg);
	cmd->cmdtype = LRCT_POS;
	cmd->key = key;
	cmd->pos[0] = x;
	cmd->pos[1] = y;
	cmd->pos[2] = z;
	s_send_and_close_rmsg(pLwc, &rmsg);
}

void rmsg_turn(LWCONTEXT* pLwc, int key, float angle) {
	zmq_msg_t rmsg;
	zmq_msg_init_size(&rmsg, sizeof(LWFIELDRENDERCOMMAND));
	LWFIELDRENDERCOMMAND* cmd = zmq_msg_data(&rmsg);
	cmd->cmdtype = LRCT_TURN;
	cmd->key = key;
	cmd->angle = angle;
	s_send_and_close_rmsg(pLwc, &rmsg);
}

void rmsg_anim(LWCONTEXT* pLwc, int key, int actionid) {
	zmq_msg_t rmsg;
	zmq_msg_init_size(&rmsg, sizeof(LWFIELDRENDERCOMMAND));
	LWFIELDRENDERCOMMAND* cmd = zmq_msg_data(&rmsg);
	cmd->cmdtype = LRCT_ANIM;
	cmd->key = key;
	cmd->actionid = actionid;
	s_send_and_close_rmsg(pLwc, &rmsg);
}

void rmsg_rparams(LWCONTEXT* pLwc, int key, int atlas, int skin_vbo, int armature, float sx, float sy, float sz) {
	zmq_msg_t rmsg;
	zmq_msg_init_size(&rmsg, sizeof(LWFIELDRENDERCOMMAND));
	LWFIELDRENDERCOMMAND* cmd = zmq_msg_data(&rmsg);
	cmd->cmdtype = LRCT_RPARAMS;
	cmd->key = key;
	cmd->atlas = atlas;
	cmd->skin_vbo = skin_vbo;
	cmd->armature = armature;
	cmd->scale[0] = sx;
	cmd->scale[1] = sy;
	cmd->scale[2] = sz;
	s_send_and_close_rmsg(pLwc, &rmsg);
}

void rmsg_quitapp(LWCONTEXT* pLwc, void* native_context) {
    zmq_msg_t rmsg;
    zmq_msg_init_size(&rmsg, sizeof(LWFIELDRENDERCOMMAND));
    LWFIELDRENDERCOMMAND* cmd = zmq_msg_data(&rmsg);
    cmd->cmdtype = LRCT_QUITAPP;
    cmd->native_context = native_context;
    s_send_and_close_rmsg(pLwc, &rmsg);
}

void rmsg_loadtex(LWCONTEXT* pLwc, int lae) {
    zmq_msg_t rmsg;
    zmq_msg_init_size(&rmsg, sizeof(LWFIELDRENDERCOMMAND));
    LWFIELDRENDERCOMMAND* cmd = zmq_msg_data(&rmsg);
    cmd->cmdtype = LRCT_LOADTEX;
    cmd->atlas = lae;
    s_send_and_close_rmsg(pLwc, &rmsg);
}

void rmsg_start_text_input_activity(LWCONTEXT* pLwc, int tag) {
    zmq_msg_t rmsg;
    zmq_msg_init_size(&rmsg, sizeof(LWFIELDRENDERCOMMAND));
    LWFIELDRENDERCOMMAND* cmd = zmq_msg_data(&rmsg);
    cmd->cmdtype = LRCT_STARTTEXTINPUTACTIVITY;
    cmd->tag = tag;
    s_send_and_close_rmsg(pLwc, &rmsg);
}

void rmsg_redraw_ui_fbo(LWCONTEXT* pLwc) {
    zmq_msg_t rmsg;
    zmq_msg_init_size(&rmsg, sizeof(LWFIELDRENDERCOMMAND));
    LWFIELDRENDERCOMMAND* cmd = zmq_msg_data(&rmsg);
    cmd->cmdtype = LRCT_REDRAWUIFBO;
    s_send_and_close_rmsg(pLwc, &rmsg);
}
