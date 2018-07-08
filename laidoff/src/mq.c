#include <czmq.h>
#include <stdlib.h>
#include "mq.h"
#include "lwlog.h"
#include "sysmsg.h"
#include "kvmsg.h"
#include "field.h"
#include "lwcontext.h"
#include "extrapolator.h"

//  Mutex macros
#if defined (__UNIX__)
typedef pthread_mutex_t zsys_mutex_t;
#   define ZMUTEX_INIT(m)    pthread_mutex_init (&m, NULL);
#   define ZMUTEX_LOCK(m)    pthread_mutex_lock (&m);
#   define ZMUTEX_UNLOCK(m)  pthread_mutex_unlock (&m);
#   define ZMUTEX_DESTROY(m) pthread_mutex_destroy (&m);
#elif defined (__WINDOWS__)
typedef CRITICAL_SECTION zsys_mutex_t;
#   define ZMUTEX_INIT(m)    InitializeCriticalSection (&m);
#   define ZMUTEX_LOCK(m)    EnterCriticalSection (&m);
#   define ZMUTEX_UNLOCK(m)  LeaveCriticalSection (&m);
#   define ZMUTEX_DESTROY(m) DeleteCriticalSection (&m);
#endif

typedef enum _LW_MESSAGE_QUEUE_STATE {
    LMQS_INIT,
    LMQS_TIME,
    LMQS_SNAPSHOT,
    LMQS_READY,
    LMQS_TERM,
} LW_MESSAGE_QUEUE_STATE;

typedef struct _LWMESSAGEQUEUE {
    zhash_t* kvmap;
    zhash_t* posmap; // UUID --> remote entity position extrapolator
    int64_t sequence;
    zsock_t* snapshot;
    zsock_t* subscriber;
    zsock_t* publisher;
    int port;
    char* subtree;
    LW_MESSAGE_QUEUE_STATE state;
    int64_t alarm;
    zuuid_t* uuid;
    int verbose;
    double delta;
    double* deltasamples;
    int deltasequence;
    int64_t start_req_time_mono;
    zsys_mutex_t mutex;
    void* bullet_counter;
    // Render message pipe writer (PAIR)
    zsock_t* rmsg_writer;
    // Render message pipe reader (PAIR)
    zsock_t* rmsg_reader;
} LWMESSAGEQUEUE;

#define DELTA_REQ_COUNT (15)
#define SUBTREE "/l/"

static void s_req_time(LWMESSAGEQUEUE* mq);

void* init_mq(const char* addr, void* sm) {
    zsys_init();

    LWMESSAGEQUEUE* mq = (LWMESSAGEQUEUE*)calloc(1, sizeof(LWMESSAGEQUEUE));
    mq->state = LMQS_INIT;
    mq->subtree = SUBTREE;
    mq->port = 5556;
    mq->verbose = 0;
    // Prepare our context and subscriber - not used in puck game
//    mq->snapshot = zsock_new(ZMQ_DEALER);
//    zsock_connect(mq->snapshot, "tcp://%s:%d", addr, mq->port);
//    mq->subscriber = zsock_new(ZMQ_SUB);
//    zsock_set_subscribe(mq->subscriber, mq->subtree);
//    zsock_connect(mq->subscriber, "tcp://%s:%d", addr, mq->port + 1);
//    mq->publisher = zsock_new(ZMQ_PUSH);
//    zsock_connect(mq->publisher, "tcp://%s:%d", addr, mq->port + 2);

    mq->kvmap = zhash_new();
    mq->posmap = zhash_new();
    mq->sequence = 0;

    mq->uuid = zuuid_new();
    mq->deltasequence = 0;
    mq->delta = 0;
    mq->deltasamples = (double*)malloc(sizeof(double) * DELTA_REQ_COUNT);

    mq->bullet_counter = zmq_atomic_counter_new();

    char sys_msg[128];
    sprintf(sys_msg, LWU("Connecting %s"), addr);
    //show_sys_msg(sm, sys_msg);

    // First we request a time sync: - not used in puck game
    //mq->start_req_time_mono = zclock_mono();
    //s_req_time(mq);

    ZMUTEX_INIT(mq->mutex);

    // Create PAIR socket for render messages
    mq->rmsg_reader = zsys_create_pipe(&mq->rmsg_writer);
    return mq;
}

static void s_req_time(LWMESSAGEQUEUE* mq) {
    zstr_sendm(mq->snapshot, "WHTTAIM?");
    char clienttime[64];
    sprintf(clienttime, "%f", mq_mono_clock());
    zstr_send(mq->snapshot, clienttime);
    mq->state = LMQS_TIME;
}

static void
s_kvmsg_free_posmap(void* ptr) {
    if (ptr) {
        LWPOSSYNCMSG* possyncmsg = (LWPOSSYNCMSG*)ptr;
        vec4_extrapolator_destroy(&possyncmsg->extrapolator);
        if (possyncmsg->field && possyncmsg->geom_index >= 0) {
            field_despawn_user(possyncmsg->field, possyncmsg->geom_index);
        }
        free(ptr);
    }
}

static void
s_kvmsg_store_posmap_noown(kvmsg_t** self_p, zhash_t* hash, double sync_time, LWMESSAGEQUEUE* mq, LWFIELD* field) {
    assert(self_p);
    if (*self_p) {
        kvmsg_t* self = *self_p;
        assert(self);
        // kvmsg_size(self) would be 0 if repeat_last is 1.
        // Any of these is nonzero, we process the message.
        if (kvmsg_size(self)) {
            // kvmsg_body may return unaligned memory address
            // which may signal SIGBUS error on Android when
            // accessing struct member variables.
            // We workaround by copying it to aligned memory address.
            LWMQMSG* msg_unaligned = (LWMQMSG*)kvmsg_body(self);
            LWMQMSG msg_aligned;
            // Should not use 'memcpy'. It is not supported on unaligned addresses.
            for (int i = 0; i < sizeof(LWMQMSG); i++) {
                ((char*)&msg_aligned)[i] = ((char*)msg_unaligned)[i];
            }
            LWMQMSG* msg = &msg_aligned;
            LWPOSSYNCMSG* possyncmsg = zhash_lookup(hash, kvmsg_key(self));
            if (possyncmsg) {
                // Do nothing
            } else {
                // Create a new extrapolator
                possyncmsg = (LWPOSSYNCMSG*)calloc(1, sizeof(LWPOSSYNCMSG));
                strcpy(possyncmsg->key, kvmsg_key(self));
                possyncmsg->x = msg->x;
                possyncmsg->y = msg->y;
                possyncmsg->z = msg->z;
                possyncmsg->extrapolator = vec4_extrapolator_new();
                // Create enemy collider if it is not the player
                if (!mq_cursor_player(mq, kvmsg_key(self))) {
                    vec3 pos = { msg->x, msg->y, msg->z };
                    possyncmsg->field = field;
                    possyncmsg->geom_index = field_spawn_user(field, pos, possyncmsg);
                }
                vec4_extrapolator_reset(possyncmsg->extrapolator, LWMIN(msg->t, sync_time) /* avoid assertion in extrapolator*/,
                    sync_time, msg->x, msg->y, msg->z, msg->dx, msg->dy);
                zhash_update(hash, kvmsg_key(self), possyncmsg);
                zhash_freefn(hash, kvmsg_key(self), s_kvmsg_free_posmap);
                if (mq->verbose) {
                    LOGI("New possyncmsg entry with key %s created.", kvmsg_key(self));
                }
            }

            if (/*mq->verbose &&*/ !mq_cursor_player(mq, kvmsg_key(self))) {
                LOGI("UPDATE: msg t %.2f sync_time %.2f, POS (%.2f, %.2f, %.2f) DXY (%.2f, %.2f)",
                    msg->t, sync_time, msg->x, msg->y, msg->z, msg->dx, msg->dy);
            }

            //possyncmsg->attacking = msg->attacking;
            //possyncmsg->moving = msg->moving;
            possyncmsg->action = msg->action;
            if (msg->stop) {
                vec4_extrapolator_add_stop(possyncmsg->extrapolator, LWMIN(msg->t, sync_time), sync_time, msg->x,
                    msg->y, msg->z, msg->dx, msg->dy);
            } else {
                vec4_extrapolator_add(possyncmsg->extrapolator, LWMIN(msg->t, sync_time), sync_time, msg->x,
                    msg->y, msg->z, msg->dx, msg->dy);
            }
            //LOGI("New possyncmsg entry with key %s updated.", kvmsg_key(self));


            if (mq->verbose && !mq_cursor_player(mq, kvmsg_key(self))) {
                float test_x, test_y, test_z, test_dx, test_dy;
                vec4_extrapolator_read(possyncmsg->extrapolator, sync_time,
                    &test_x, &test_y, &test_z, &test_dx, &test_dy);
                LOGI("UPDATE-SAMPLED: t POS (%.2f, %.2f, %.2f) DXY (%.2f, %.2f)",
                    test_x, test_y, test_z, test_dx, test_dy);
            }

        } else {
            zhash_delete(hash, kvmsg_key(self));
        }
    }
}

static int s_delta_cmp(const void* a, const void* b) {
    double d = *(double*)a - *(double*)b;
    return d > 0 ? 1 : d < 0 ? -1 : 0;
}

static void s_mq_poll_time(void* _mq, void* sm) {
    LWMESSAGEQUEUE* mq = (LWMESSAGEQUEUE*)_mq;
    zmq_pollitem_t items[] = { { zsock_resolve(mq->snapshot), 0, ZMQ_POLLIN, 0 } };
    int rc = zmq_poll(items, 1, 0);
    if (rc == -1) {
        return;
    }

    if (items[0].revents & ZMQ_POLLIN) {
        char* name = zstr_recv(mq->snapshot);
        if (name) {
            free(name);
            char* timereply = zstr_recv(mq->snapshot);
            if (timereply) {
                // https://en.wikipedia.org/wiki/Network_Time_Protocol#Clock_synchronization_algorithm
                double t2 = 0, t1 = 0, t0 = 0;
                sscanf(timereply, "%lf %lf %lf", &t2, &t1, &t0);
                double t3 = mq_mono_clock();
                double delta = ((t1 - t0) + (t2 - t3)) / 2;
                mq->deltasamples[mq->deltasequence++] = delta;
                double roundtrip_delay = (t3 - t0) - (t2 - t1);

                /*if (mq->verbose)*/
                {
                    LOGI("REQ %d: delta = %f sec, rtd = %f sec", mq->deltasequence, delta, roundtrip_delay);
                }

                char sys_msg[128];
                sprintf(sys_msg, LWU("Timesyncing %d/%d"), mq->deltasequence, DELTA_REQ_COUNT);
                show_sys_msg(sm, sys_msg);
                // Request for another or finish
                if (mq->deltasequence < DELTA_REQ_COUNT) {
                    s_req_time(mq);
                } else {
                    int64_t req_time_elapsed = zclock_mono() - mq->start_req_time_mono;
                    LOGI("Tymesyncing duration: %f sec (%f sec / req)", req_time_elapsed / 1e3, req_time_elapsed / 1e3 / DELTA_REQ_COUNT);

                    // Calculate averaged delta around a median delta:
                    qsort(mq->deltasamples, DELTA_REQ_COUNT, sizeof(mq->deltasamples[0]), s_delta_cmp);
                    int nselsamples = 10;
                    int ibeg = LWMAX(0, DELTA_REQ_COUNT / 2 - nselsamples / 2);
                    int iend = LWMIN(DELTA_REQ_COUNT / 2 + nselsamples / 2, DELTA_REQ_COUNT);
                    double selsum = 0;
                    for (int i = ibeg; i < iend; i++) {
                        selsum += mq->deltasamples[i];
                    }
                    mq->delta = selsum / nselsamples;
                    LOGI("FINAL delta = %f", mq->delta);

                    // Next, we request a state snapshot:
                    zstr_sendm(mq->snapshot, "ICANHAZ?");
                    zstr_send(mq->snapshot, mq->subtree);
                    mq->state = LMQS_SNAPSHOT;
                }
            }
        }
    }
}

static void s_mq_poll_snapshot(void* _mq, void* sm, LWFIELD* field) {
    LWMESSAGEQUEUE* mq = (LWMESSAGEQUEUE*)_mq;
    zmq_pollitem_t items[] = { { zsock_resolve(mq->snapshot), 0, ZMQ_POLLIN, 0 } };
    int rc = zmq_poll(items, 1, 0);
    if (rc == -1) {
        return;
    }

    if (items[0].revents & ZMQ_POLLIN) {
        kvmsg_t* kvmsg = kvmsg_recv(zsock_resolve(mq->snapshot));
        if (!kvmsg) {
            return;
        }
        if (streq(kvmsg_key(kvmsg), "KTHXBAI")) {
            mq->sequence = kvmsg_sequence(kvmsg);
            LOGI("received snapshot sequence %"PRId64, mq->sequence);
            kvmsg_destroy(&kvmsg);
            mq->state = LMQS_READY;
            mq->alarm = zclock_time() + 100;
            show_sys_msg(sm, LWU("Snapshot downloaded"));
            return;
        }
        show_sys_msg(sm, LWU("Receiving snapshot"));
        mq_lock_mutex(mq);
        s_kvmsg_store_posmap_noown(&kvmsg, mq->posmap, mq_sync_mono_clock(mq), mq, field);
        mq_unlock_mutex(mq);
        kvmsg_store(&kvmsg, mq->kvmap);
    }
}

static void s_send_pos(const LWCONTEXT* pLwc, LWMESSAGEQUEUE* mq, int stop) {
    // Should not send position before timesync is completed.
    if (mq->state != LMQS_READY) {
        return;
    }

    kvmsg_t* kvmsg = kvmsg_new(0);
    kvmsg_fmt_key(kvmsg, "%s%s", mq->subtree, zuuid_str(mq->uuid));
    LWMQMSG msg;
    memset(&msg, 0, sizeof(LWMQMSG));
    get_field_player_position(pLwc->field, &msg.x, &msg.y, &msg.z);
    msg.dx = pLwc->player_pos_last_moved_dx;
    msg.dy = pLwc->player_pos_last_moved_dy;
    //msg.moving = pLwc->player_moving;
    //msg.attacking = pLwc->player_attacking;
    msg.stop = stop;
    msg.action = get_anim_by_state(pLwc->player_state_data.state, 0);
    msg.t = mq_sync_mono_clock(mq);
    kvmsg_set_prop(kvmsg, "ttl", "%d", 2); // Set TTL to 2 seconds.
    kvmsg_set_body(kvmsg, (byte*)&msg, sizeof(msg));
    kvmsg_send(kvmsg, zsock_resolve(mq->publisher));
    kvmsg_destroy(&kvmsg);
    if (mq->verbose) {
        LOGI("s_send_pos");
    }
}

int mq_bullet_counter(LWMESSAGEQUEUE* mq) {
    return zmq_atomic_counter_inc(mq->bullet_counter);
}

void mq_send_fire(LWMESSAGEQUEUE* mq, const float* pos, const float* vel) {
    kvmsg_t* kvmsg = kvmsg_new(0);
    kvmsg_fmt_key(kvmsg, "%s%s/%s/tznt", mq->subtree, zuuid_str(mq->uuid), "fire");
    LWFIREMSG msg;
    memset(&msg, 0, sizeof(LWFIREMSG));
    msg.type = 0x01;
    memcpy(msg.pos, pos, sizeof(msg.pos));
    memcpy(msg.vel, vel, sizeof(msg.vel));
    strcpy(msg.owner_key, zuuid_str(mq->uuid));
    msg.bullet_id = mq_bullet_counter(mq);
    kvmsg_set_body(kvmsg, (byte*)&msg, sizeof(msg));
    kvmsg_send(kvmsg, zsock_resolve(mq->publisher));
    kvmsg_destroy(&kvmsg);
    if (mq->verbose) {
        LOGI("mq_send_fire");
    }
}

void mq_send_action(LWMESSAGEQUEUE* mq, int action) {
    if (mq->publisher == 0) {
        return;
    }
    kvmsg_t* kvmsg = kvmsg_new(0);
    kvmsg_fmt_key(kvmsg, "%s%s/%s/tznt", mq->subtree, zuuid_str(mq->uuid), "action");
    LWACTIONMSG msg;
    msg.type = 0x02;
    msg.action = action;
    kvmsg_set_body(kvmsg, (byte*)&msg, sizeof(msg));
    kvmsg_send(kvmsg, zsock_resolve(mq->publisher));
    kvmsg_destroy(&kvmsg);
    if (mq->verbose) {
        LOGI("mq_send_action");
    }
}

static void s_mq_poll_ready(void* _pLwc, void* _mq, void* sm, void* field) {
    LWMESSAGEQUEUE* mq = (LWMESSAGEQUEUE*)_mq;
    LWCONTEXT* pLwc = (LWCONTEXT*)_pLwc;
    zmq_pollitem_t items[] = { { zsock_resolve(mq->subscriber), 0, ZMQ_POLLIN, 0 } };
    
    zmq_poller_event_t *events;
    void *poller;
    zmq_poll_noalloc_prep(1, &events, &poller);
    zmq_poller_prep(items, 1, poller);
    const char* player_uuid = zuuid_str(mq->uuid);

    const size_t subtree_len = strlen(SUBTREE);

    // Process all queued messages at once
    while (zmq_poll_noalloc(items, 1, 0, events, poller) > 0) {
        // Polling subscriber socket to get update
        if (items[0].revents & ZMQ_POLLIN) {
            kvmsg_t* kvmsg = kvmsg_recv(zsock_resolve(mq->subscriber));
            if (!kvmsg) {
                return;
            }

            size_t key_len = strlen(kvmsg_key(kvmsg));
            const static char* TRANSIENT = "/tznt";
            size_t transient_postfix_len = strlen(TRANSIENT);

            size_t msg_size = kvmsg_size(kvmsg);
            void* msg_unaligned = kvmsg_body(kvmsg);
            char msg_aligned[2048];
            if (msg_size > sizeof(msg_aligned)) {
                LOGEP("msg_size exceeded (%zd) maximum (%zd)", msg_size, sizeof(msg_aligned));
            }
            // Should not use 'memcpy'. It is not supported on unaligned addresses.
            for (size_t i = 0; i < msg_size; i++) {
                msg_aligned[i] = ((char*)msg_unaligned)[i];
            }

            if (streq(kvmsg_key(kvmsg), SUBTREE "announce")) {
                show_sys_msg(sm, (char*)msg_aligned); // kvmsg body is assumed to be a null terminated string.
                kvmsg_destroy(&kvmsg);
            } else if (key_len > transient_postfix_len && streq(kvmsg_key(kvmsg) + (key_len - transient_postfix_len), TRANSIENT)) {
                int type = *(int*)kvmsg_body(kvmsg);
                switch (type) {
                case 0x01:
                {
                    LWFIREMSG* msg = (LWFIREMSG*)msg_aligned;
                    if (strcmp(msg->owner_key, player_uuid) == 0) {
                        // My(player) bullet
                        field_spawn_sphere(field, msg->pos, msg->vel, msg->bullet_id);
                    } else {
                        // Enemy bullet
                        field_spawn_remote_sphere(field, msg->pos, msg->vel, msg->bullet_id, msg->owner_key);
                    }
                    break;
                }
                case 0x02:
                {
                    //LWACTIONMSG* msg = (LWACTIONMSG*)msg_aligned;
                    // TODO Update action field only
                    break;
                }
                case 0x03:
                {
                    LWHITMSG* msg = (LWHITMSG*)msg_aligned;
                    
                    if (strcmp(msg->target_key + subtree_len, player_uuid) == 0) {
                        // Enemy bullet hitting me
                        field_hit_player(field);
                    } else {
                        // Enemy bullet hitting other enemy
                        LWPOSSYNCMSG* possyncmsg = zhash_lookup(mq->posmap, msg->target_key);
                        if (possyncmsg) {
                            possyncmsg->flash = 1.0f;
                        }
                    }
                    break;
                }
                case 0x04:
                {
                    LWDESPAWNBULLETMSG* msg = (LWDESPAWNBULLETMSG*)msg_aligned;
                    field_despawn_remote_sphere(field, msg->bullet_id, msg->owner_key);
                    break;
                }
                default:
                {
                    LOGE("Unknown transient message received: type %d", type);
                    break;
                }
                }
                kvmsg_destroy(&kvmsg);
            } else {
                // Discard out-of-sequence kvmsgs, incl. heartbeats
                if (kvmsg_sequence(kvmsg) > mq->sequence) {
                    mq->sequence = kvmsg_sequence(kvmsg);
                    if (mq->verbose) {
                        LOGI("Update: %"PRId64" key:%s, bodylen:%zd\n", mq->sequence, kvmsg_key(kvmsg), kvmsg_size(kvmsg));
                    }
                    mq_lock_mutex(mq);
                    s_kvmsg_store_posmap_noown(&kvmsg, mq->posmap, mq_sync_mono_clock(mq), mq, field);
                    mq_unlock_mutex(mq);
                    kvmsg_store(&kvmsg, mq->kvmap);
                } else {
                    kvmsg_destroy(&kvmsg);
                }
            }
        }
    }
    zmq_poll_noalloc_unprep(&events, &poller);
    
    if (zclock_time() >= mq->alarm) {
        s_send_pos(pLwc, mq, 0);
        mq->alarm = zclock_time() + 200;
    }
}

void* mq_kvmap(void* _mq) {
    LWMESSAGEQUEUE* mq = (LWMESSAGEQUEUE*)_mq;
    return mq->kvmap;
}

const LWMQMSG* mq_sync_first(void* _mq) {
    LWMESSAGEQUEUE* mq = (LWMESSAGEQUEUE*)_mq;
    kvmsg_t* value = (kvmsg_t*)zhash_first(mq->kvmap);
    if (value) {
        return (const LWMQMSG*)kvmsg_body(value);
    }
    return 0;
}

const char* mq_sync_cursor(void* _mq) {
    LWMESSAGEQUEUE* mq = (LWMESSAGEQUEUE*)_mq;
    return zhash_cursor(mq->kvmap);
}

const LWMQMSG* mq_sync_next(void* _mq) {
    LWMESSAGEQUEUE* mq = (LWMESSAGEQUEUE*)_mq;
    kvmsg_t* value = (kvmsg_t*)zhash_next(mq->kvmap);
    if (value) {
        return (const LWMQMSG*)kvmsg_body(value);
    }
    return 0;
}

LWPOSSYNCMSG* mq_possync_first(void* _mq) {
    if (_mq == 0) {
        return 0;
    }
    LWMESSAGEQUEUE* mq = (LWMESSAGEQUEUE*)_mq;
    return (LWPOSSYNCMSG*)zhash_first(mq->posmap);
}

const char* mq_possync_cursor(void* _mq) {
    LWMESSAGEQUEUE* mq = (LWMESSAGEQUEUE*)_mq;
    return zhash_cursor(mq->posmap);
}

LWPOSSYNCMSG* mq_possync_next(void* _mq) {
    LWMESSAGEQUEUE* mq = (LWMESSAGEQUEUE*)_mq;
    return (LWPOSSYNCMSG*)zhash_next(mq->posmap);
}

const char* mq_uuid_str(void* _mq) {
    LWMESSAGEQUEUE* mq = (LWMESSAGEQUEUE*)_mq;
    return zuuid_str(mq->uuid);
}

const char* mq_subtree(void* _mq) {
    LWMESSAGEQUEUE* mq = (LWMESSAGEQUEUE*)_mq;
    return mq->subtree;
}

void mq_poll(void* _pLwc, void* sm, void* _mq, void* field) {
    LWMESSAGEQUEUE* mq = (LWMESSAGEQUEUE*)_mq;
    switch (mq->state) {
    case LMQS_INIT:
        break;
    case LMQS_TIME:
        s_mq_poll_time(mq, sm);
        break;
    case LMQS_SNAPSHOT:
        s_mq_poll_snapshot(mq, sm, field);
        break;
    case LMQS_READY:
        s_mq_poll_ready(_pLwc, mq, sm, field);
        break;
    default:
        break;
    }
}

void deinit_mq(void* _mq) {
    LWMESSAGEQUEUE* mq = (LWMESSAGEQUEUE*)_mq;
    if (mq == 0) {
        return;
    }
    zhash_destroy(&mq->kvmap);
    zhash_destroy(&mq->posmap);
    zsock_destroy(&mq->publisher);
    zsock_destroy(&mq->snapshot);
    zsock_destroy(&mq->subscriber);
    zuuid_destroy(&mq->uuid);
    ZMUTEX_DESTROY(mq->mutex);
    zsock_destroy(&mq->rmsg_reader);
    zsock_destroy(&mq->rmsg_writer);
    zmq_atomic_counter_destroy(&mq->bullet_counter);
    mq->state = LMQS_TERM;
    free(mq);
}

void mq_shutdown() {
    zsys_shutdown();
}

void mq_publish_now(void* pLwc, void* _mq, int stop) {
    LWMESSAGEQUEUE* mq = (LWMESSAGEQUEUE*)_mq;
    s_send_pos(pLwc, mq, stop);
    //mq->alarm = 0;
}

double mq_mono_clock() {
    return zclock_mono() / 1e3;
}

double mq_sync_mono_clock(void* _mq) {
    LWMESSAGEQUEUE* mq = (LWMESSAGEQUEUE*)_mq;
    return mq_mono_clock() + mq->delta;
}

int mq_cursor_player(void* _mq, const char* cursor) {
    LWMESSAGEQUEUE* mq = (LWMESSAGEQUEUE*)_mq;
    return strcmp(cursor + strlen(mq_subtree(mq)), mq_uuid_str(mq)) == 0;
}

void mq_interrupt() {
    zsys_interrupted = 1;
    zctx_interrupted = 1;
}

void mq_lock_mutex(void* _mq) {
    if (_mq == 0) {
        return;
    }
    LWMESSAGEQUEUE* mq = (LWMESSAGEQUEUE*)_mq;
    ZMUTEX_LOCK(mq->mutex);
}

void mq_unlock_mutex(void* _mq) {
    if (_mq == 0) {
        return;
    }
    LWMESSAGEQUEUE* mq = (LWMESSAGEQUEUE*)_mq;
    ZMUTEX_UNLOCK(mq->mutex);
}

void mq_send_hit(void* _mq, LWPOSSYNCMSG* possyncmsg) {
    LWMESSAGEQUEUE* mq = (LWMESSAGEQUEUE*)_mq;
    kvmsg_t* kvmsg = kvmsg_new(0);
    kvmsg_fmt_key(kvmsg, "%s%s/%s/tznt", mq->subtree, zuuid_str(mq->uuid), "hit");
    LWHITMSG msg;
    memset(&msg, 0, sizeof(LWHITMSG));
    msg.type = 0x03;
    strcpy(msg.target_key, possyncmsg->key);
    kvmsg_set_body(kvmsg, (byte*)&msg, sizeof(msg));
    kvmsg_send(kvmsg, zsock_resolve(mq->publisher));
    kvmsg_destroy(&kvmsg);
    if (mq->verbose) {
        LOGI(__func__);
    }
}

void mq_send_despawn_bullet(void* _mq, int bullet_id) {
    LWMESSAGEQUEUE* mq = (LWMESSAGEQUEUE*)_mq;
    kvmsg_t* kvmsg = kvmsg_new(0);
    kvmsg_fmt_key(kvmsg, "%s%s/%s/tznt", mq->subtree, zuuid_str(mq->uuid), "despawnbullet");
    LWDESPAWNBULLETMSG msg;
    memset(&msg, 0, sizeof(LWDESPAWNBULLETMSG));
    msg.type = 0x04;
    msg.bullet_id = bullet_id;
    strcpy(msg.owner_key, zuuid_str(mq->uuid));
    kvmsg_set_body(kvmsg, (byte*)&msg, sizeof(msg));
    kvmsg_send(kvmsg, zsock_resolve(mq->publisher));
    kvmsg_destroy(&kvmsg);
    if (mq->verbose) {
        LOGI(__func__);
    }
}

void* mq_rmsg_reader(void* _mq) {
    if (_mq == 0) {
        return 0;
    }
    LWMESSAGEQUEUE* mq = (LWMESSAGEQUEUE*)_mq;
    return zsock_resolve(mq->rmsg_reader);
}

void* mq_rmsg_writer(void* _mq) {
    if (_mq == 0) {
        return 0;
    }
    LWMESSAGEQUEUE* mq = (LWMESSAGEQUEUE*)_mq;
    return zsock_resolve(mq->rmsg_writer);
}
