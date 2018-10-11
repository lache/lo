#include "lwtcp.h"
#include "numcomp_puck_game.h"
#include "lwlog.h"
#include "puckgame.h"
#include "puckgamerecord.h"
#include "lwtimepoint.h"
#include <tinycthread.h>
#include "lwmacro.h"
#if LW_PLATFORM_WIN32
#   include <winsock2.h>
#elif !LW_PLATFORM_OSX
#   include <endian.h>
#endif
#include "lz4.h"

#if LW_PLATFORM_WIN32
#   define LwChangeDirectory(x) SetCurrentDirectory(x)
#else
#   define LwChangeDirectory(x) chdir(x)
#endif

#ifndef BOOL
#   define BOOL int
#endif

#ifndef INVALID_SOCKET
#   define INVALID_SOCKET (~0)
#endif

#ifndef SOCKET_ERROR
#   define SOCKET_ERROR (-1)
#endif

#define BUFLEN 512  //Max length of buffer
#define PORT 10288   //The port on which to listen for incoming data

#define TCP_INTERNAL_PORT 29856
#define LW_PUCK_GAME_POOL_CAPACITY (512)

typedef struct _LWSERVER {
    SOCKET s;
    struct sockaddr_in server, si_other;
    socklen_t slen;
    int recv_len;
    char buf[BUFLEN];
#if LW_PLATFORM_WIN32
    WSADATA wsa;
#endif
    int broadcast_count;
    int token_counter;
    int battle_counter;
    LWPUCKGAME* puck_game_pool[LW_PUCK_GAME_POOL_CAPACITY];
    LWNUMCOMPPUCKGAME numcomp;
    int update_frequency;
} LWSERVER;

typedef struct _LWTCPSERVER {
    SOCKET s;
    struct sockaddr_in server, si_other;
    int slen;
    char buf[BUFLEN];
#if LW_PLATFORM_WIN32
    WSADATA wsa;
#endif
} LWTCPSERVER;

static BOOL directory_exists(const char* szPath) {
#if LW_PLATFORM_WIN32
    DWORD dwAttrib = GetFileAttributes(szPath);

    return (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
#else
    DIR* dir = opendir(szPath);
    if (dir) {
        return 1;
    } else {
        return 0;
    }
#endif
}

static int update_puck_game(LWSERVER* server, LWPUCKGAME* puck_game, double delta_time) {
    // update tick (server-client shared portion)
    puck_game_update_tick(puck_game, server->update_frequency);
    // check for termination condition
    if (puck_game_state_phase_finished(puck_game->battle_phase)) {
        return -1;
    }
    if (puck_game->bogus_opponent) {
        // easy version first
        LWPUCKGAMEBOGUSPARAM bogus_param = {
                0.0075f, // target_follow_agility
                0.30f, // dash_detect_radius
                0.2f, // dash_frequency
                0.8f, // dash_cooltime_lag_min
                1.2f, // dash_cooltime_lag_max
        };
        if (puck_game_state_phase_battling(puck_game->battle_phase)) {
            puck_game_control_bogus(puck_game, &bogus_param);
        }
    }
    for (int i = 0; i < 2; i++) {
        puck_game_update_remote_player(puck_game, (float)delta_time, i);
    }
    update_puck_ownership(puck_game);
    update_puck_reflect_size(puck_game, (float)delta_time);
    return 0;
}

LWSERVER* new_server() {
    LWSERVER* server = malloc(sizeof(LWSERVER));
    memset(server, 0, sizeof(LWSERVER));
    server->slen = sizeof(server->si_other);
#if LW_PLATFORM_WIN32
    // Initialize winsock
    LOGI("Initializing Winsock...");
    if (WSAStartup(MAKEWORD(2, 2), &server->wsa) != 0)
    {
        printf("Failed. Error Code : %d", WSAGetLastError());
        exit(EXIT_FAILURE);
    }
    LOGI("Initialized.\n");
#endif
    //Create a socket
    if ((server->s = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET) {
        LOGE("Could not create socket : %d", WSAGetLastError());
    }
    LOGI("Socket created.\n");

    //Prepare the sockaddr_in structure
    server->server.sin_family = AF_INET;
    server->server.sin_addr.s_addr = INADDR_ANY;
    server->server.sin_port = htons(PORT);

    //Bind
    if (bind(server->s, (struct sockaddr*)&server->server, sizeof(server->server)) == SOCKET_ERROR) {
        LOGE("Bind failed with error code : %d", WSAGetLastError());
        exit(EXIT_FAILURE);
    }
    LOGI("Bind done");
    // numcomp presets setup
    numcomp_puck_game_init(&server->numcomp);
    return server;
}

LWTCPSERVER* new_tcp_server() {
    LWTCPSERVER* server = malloc(sizeof(LWTCPSERVER));
    memset(server, 0, sizeof(LWTCPSERVER));
    server->slen = sizeof(server->si_other);
    //Create a socket
    if ((server->s = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        LOGE("Could not create tcp socket : %d", WSAGetLastError());
    }
    LOGI("TCP Socket created.\n");

    if (setsockopt(server->s, SOL_SOCKET, SO_REUSEADDR, (const char*)&(int){1}, sizeof(int)) < 0) {
        LOGE("setsockopt(SO_REUSEADDR) failed");
    }

    //Prepare the sockaddr_in structure
    server->server.sin_family = AF_INET;
    server->server.sin_addr.s_addr = INADDR_ANY;
    server->server.sin_port = htons(TCP_INTERNAL_PORT);

    //Bind
    if (bind(server->s, (struct sockaddr*)&server->server, sizeof(server->server)) == SOCKET_ERROR) {
        LOGE("TCP Bind failed with error code : %d", WSAGetLastError());
        exit(EXIT_FAILURE);
    }
    LOGI("TCP Bind done");
    listen(server->s, 3);
    LOGI("TCP Listening...");
    return server;
}

int make_socket_nonblocking(int sock) {
#if defined(WIN32) || defined(_WIN32) || defined(IMN_PIM)
    unsigned long arg = 1;
    return ioctlsocket(sock, FIONBIO, &arg) == 0;
#elif defined(VXWORKS)
    int arg = 1;
    return ioctl(sock, FIONBIO, (int)&arg) == 0;
#else
    int curFlags = fcntl(sock, F_GETFL, 0);
    return fcntl(sock, F_SETFL, curFlags | O_NONBLOCK) >= 0;
#endif
}

void server_send(LWSERVER* server, const char* p, int s) {
    sendto(server->s, p, (size_t)s, 0, (struct sockaddr*)&server->si_other, server->slen);
}

#define SERVER_SEND(server, packet) server_send(server, (const char*)&(packet), sizeof(packet))

typedef struct _LWCONN {
    unsigned long long ipport;
    struct sockaddr_in si;
    double last_ingress_timepoint;
    int battle_id;
    unsigned int token;
    int player_no;
} LWCONN;

#define LW_CONN_CAPACITY (32)

unsigned long long to_ipport(unsigned int ip, unsigned short port) {
    return ((unsigned long long)port << 32) | ip;
}

void add_conn(LWCONN* conn, int conn_capacity, struct sockaddr_in* si) {
    unsigned long long ipport = to_ipport(si->sin_addr.s_addr, si->sin_port);
    // Update last ingress for existing element
    for (int i = 0; i < conn_capacity; i++) {
        if (conn[i].ipport == ipport) {
            conn[i].last_ingress_timepoint = lwtimepoint_now_seconds();
            return;
        }
    }
    // New element
    for (int i = 0; i < conn_capacity; i++) {
        if (conn[i].ipport == 0) {
            conn[i].ipport = ipport;
            conn[i].last_ingress_timepoint = lwtimepoint_now_seconds();
            memcpy(&conn[i].si, si, sizeof(struct sockaddr_in));
            return;
        }
    }
    LOGE("add_conn: maximum capacity exceeded.");
}

void add_conn_with_token(LWCONN* conn,
                         int conn_capacity,
                         struct sockaddr_in* si,
                         int battle_id,
                         unsigned int token,
                         int player_no) {
    unsigned long long ipport = to_ipport(si->sin_addr.s_addr, si->sin_port);
    // Update last ingress for existing element
    for (int i = 0; i < conn_capacity; i++) {
        if (conn[i].ipport == ipport) {
            conn[i].last_ingress_timepoint = lwtimepoint_now_seconds();
            conn[i].battle_id = battle_id;
            conn[i].token = token;
            conn[i].player_no = player_no;
            return;
        }
    }
    LOGE("add_conn_with_token: not exist...");
}

void invalidate_dead_conn(LWCONN* conn,
                          int conn_capacity,
                          double current_timepoint,
                          double life) {
    for (int i = 0; i < conn_capacity; i++) {
        if (conn[i].ipport) {
            if (current_timepoint - conn[i].last_ingress_timepoint > life) {
                conn[i].ipport = 0;
                conn[i].battle_id = 0;
            }
        }
    }
}

int check_token(LWSERVER* server,
                LWPUDPHEADER* p,
                LWPUCKGAME** puck_game) {
    if ((p->battle_id < 1) || p->battle_id > LW_PUCK_GAME_POOL_CAPACITY) {
        if (p->battle_id != 0) {
            LOGE("Invalid battle id range: %d", p->battle_id);
        } else {
            // p->battle_id == 0 every time client runs
        }
        return -1;
    }
    LWPUCKGAME* pg = server->puck_game_pool[p->battle_id - 1];
    if (!pg) {
        //LOGE("Battle id %d is null.", p->battle_id);
        return -2;
    }
    if (pg->c1_token == p->token) {
        *puck_game = pg;
        return 1;
    } else if (pg->c2_token == p->token) {
        *puck_game = pg;
        return 2;
    }
    LOGE("Battle id %d token not match.", p->battle_id);
    return -3;
}

int check_player_no(int player_no) {
    return player_no == 1 || player_no == 2;
}

int control_index_from_player_no(int player_no) {
    return player_no - 1;
}

void select_server(LWSERVER* server, LWCONN* conn, LWTCP* reward_service) {
    //clear the buffer by filling null, it might have previously received data
    memset(server->buf, '\0', BUFLEN);
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(server->s, &readfds);
    if (reward_service && reward_service->connect_socket != INVALID_SOCKET) {
        FD_SET(reward_service->connect_socket, &readfds);
    }
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 2500; // 400 Hz
    int maxfds = (int)(LWMAX(reward_service->connect_socket, server->s)) + 1;
    int rv = select(maxfds, &readfds, NULL, NULL, &tv);
    LOGIx("select() return value: %d", rv);
    //try to receive some data, this is a blocking call
    switch (rv) {
        case -1:
            LOGEP("select() returns -1");
            exit(-2);
//        case 0:
//            // timeout
//            break;
        default:
            if (reward_service
                && reward_service->connect_socket != INVALID_SOCKET
                && FD_ISSET(reward_service->connect_socket, &readfds)) {
                int recv_count = (int)recv(reward_service->connect_socket,
                                           reward_service->recv_buf,
                                           (size_t)reward_service->recv_buf_len,
                                           MSG_DONTWAIT);
                if (recv_count == 0) {
                    LOGEP("Recv size 0 from reward server. Disconnected.");
                    closesocket(reward_service->connect_socket);
                    reward_service->connect_socket = INVALID_SOCKET;
                }
            }
            if (FD_ISSET(server->s, &readfds)) {
                server->recv_len = (int)recvfrom(server->s,
                                                 server->buf,
                                                 BUFLEN,
                                                 0,
                                                 (struct sockaddr*)&server->si_other,
                                                 &server->slen);
                if (server->recv_len == SOCKET_ERROR) {

                } else {
                    add_conn(conn, LW_CONN_CAPACITY, &server->si_other);

                    const int packet_type = *(int*)server->buf;
                    switch (packet_type) {
                        case LPGP_LWPGETTOKEN: {
                            LWPTOKEN p;
                            p.type = LPGP_LWPTOKEN;
                            ++server->token_counter;
                            p.token = server->token_counter;
                            SERVER_SEND(server, p);
                            break;
                        }
                        case LPGP_LWPQUEUE: {
                            LWPMATCHED p;
                            p.type = LPGP_LWPMATCHED;
                            p.master = 0;
                            SERVER_SEND(server, p);
                            break;
                        }
                        case LPGP_LWPMOVE: {
                            LWPMOVE* p = (LWPMOVE*)server->buf;
                            LWPUCKGAME* pg = 0;
                            int player_no = check_token(server, (LWPUDPHEADER*)p, &pg);
                            if (check_player_no(player_no)) {
                                //LOGI("MOVE dx=%.2f dy=%.2f", p->dx, p->dy);
                                LWREMOTEPLAYERCONTROL* control = &pg->remote_control[control_index_from_player_no(player_no)][0];
                                control->dir_pad_dragging = 1;
                                control->dx = p->dx;
                                control->dy = p->dy;
                                control->dlen = p->dlen;
                                add_conn_with_token(conn,
                                                    LW_CONN_CAPACITY,
                                                    &server->si_other,
                                                    p->battle_id,
                                                    (unsigned int)p->token,
                                                    player_no);
                            }
                            break;
                        }
                        case LPGP_LWPSTOP: {
                            LWPSTOP* p = (LWPSTOP*)server->buf;
                            LWPUCKGAME* pg = 0;
                            int player_no = check_token(server, (LWPUDPHEADER*)p, &pg);
                            if (check_player_no(player_no)) {
                                //LOGI("STOP");
                                LWREMOTEPLAYERCONTROL* control = &pg->remote_control[control_index_from_player_no(player_no)][0];
                                control->dir_pad_dragging = 0;
                                add_conn_with_token(conn,
                                                    LW_CONN_CAPACITY,
                                                    &server->si_other,
                                                    p->battle_id,
                                                    (unsigned int)p->token,
                                                    player_no);
                            }
                            break;
                        }
                        case LPGP_LWPDASH: {
                            LWPDASH* p = (LWPDASH*)server->buf;
                            LWPUCKGAME* pg = 0;
                            int player_no = check_token(server, (LWPUDPHEADER*)p, &pg);
                            if (check_player_no(player_no)) {
                                //LOGI("DASH");
                                LWPUCKGAMEDASH* dash = &pg->remote_dash[control_index_from_player_no(player_no)][0];
                                puck_game_commit_dash_to_puck(pg, dash, player_no);
                                add_conn_with_token(conn,
                                                    LW_CONN_CAPACITY,
                                                    &server->si_other,
                                                    p->battle_id,
                                                    (unsigned int)p->token,
                                                    player_no);
                            }
                            break;
                        }
                        case LPGP_LWPJUMP: {
                            LWPJUMP* p = (LWPJUMP*)server->buf;
                            LWPUCKGAME* pg = 0;
                            int player_no = check_token(server, (LWPUDPHEADER*)p, &pg);
                            if (check_player_no(player_no)) {
                                //LOGI("JUMP");
                                LWPUCKGAMEJUMP* jump = &pg->remote_jump[control_index_from_player_no(player_no)][0];
                                puck_game_commit_jump(pg, jump, player_no);
                                add_conn_with_token(conn,
                                                    LW_CONN_CAPACITY,
                                                    &server->si_other,
                                                    p->battle_id,
                                                    (unsigned int)p->token,
                                                    player_no);
                            }
                            break;
                        }
                        case LPGP_LWPFIRE: {
                            LWPFIRE* p = (LWPFIRE*)server->buf;
                            LWPUCKGAME* pg = 0;
                            int player_no = check_token(server, (LWPUDPHEADER*)p, &pg);
                            if (check_player_no(player_no)) {
                                //LOGI("FIRE");
                                LWPUCKGAMEFIRE* fire = &pg->remote_fire[control_index_from_player_no(player_no)][0];
                                puck_game_commit_fire(pg, fire, player_no, p->dx, p->dy, p->dlen);
                                add_conn_with_token(conn,
                                                    LW_CONN_CAPACITY,
                                                    &server->si_other,
                                                    p->battle_id,
                                                    (unsigned int)p->token,
                                                    player_no);
                            }
                            break;
                        }
                        case LPGP_LWPPULLSTART: {
                            LWPPULLSTART* p = (LWPPULLSTART*)server->buf;
                            LWPUCKGAME* pg = 0;
                            int player_no = check_token(server, (LWPUDPHEADER*)p, &pg);
                            if (check_player_no(player_no)) {
                                //LOGI("PULL START");
                                LWREMOTEPLAYERCONTROL* control = &pg->remote_control[control_index_from_player_no(player_no)][0];
                                control->pull_puck = 1;
                                add_conn_with_token(conn,
                                                    LW_CONN_CAPACITY,
                                                    &server->si_other,
                                                    p->battle_id,
                                                    (unsigned int)p->token,
                                                    player_no);
                            }
                            break;
                        }
                        case LPGP_LWPPULLSTOP: {
                            LWPPULLSTOP* p = (LWPPULLSTOP*)server->buf;
                            LWPUCKGAME* pg = 0;
                            int player_no = check_token(server, (LWPUDPHEADER*)p, &pg);
                            if (check_player_no(player_no)) {
                                //LOGI("PULL STOP");
                                LWREMOTEPLAYERCONTROL* control = &pg->remote_control[control_index_from_player_no(player_no)][0];
                                control->pull_puck = 0;
                                add_conn_with_token(conn,
                                                    LW_CONN_CAPACITY,
                                                    &server->si_other,
                                                    p->battle_id,
                                                    (unsigned int)p->token,
                                                    player_no);
                            }
                            break;
                        }
                        default: {
                            break;
                        }
                    }
                }
            }

            break;
    }
}

static char* server_create_binary_from_file(const char* filename, size_t* size) {
    FILE* f;
    f = fopen(filename, "rb");
    if (f) {
        fseek(f, 0, SEEK_END);
        const int f_size = (int)ftell(f);
        fseek(f, 0, SEEK_SET);
        char* d = (char*)malloc(f_size);
        fread(d, 1, f_size, f);
        *size = f_size;
        fclose(f);
        LOGI("%s: %s (%d bytes) loaded to memory.", __func__, filename, f_size);
        return d;
    }
    LOGE("%s: %s [ERROR] FILE NOT FOUND.", __func__, filename);
    return 0;
}

int tcp_admin_server_entry(void* context) {
    LWTCPSERVER* tcp_server = new_tcp_server();
    LWSERVER* server = context;
#if !LW_PLATFORM_WIN32
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
#endif
    while (1) {
        int c = sizeof(struct sockaddr_in);
        SOCKET client_sock = accept(tcp_server->s, (struct sockaddr*)&tcp_server->server, (socklen_t*)&c);
        char recv_buf[512];
        int recv_len = (int)recv(client_sock, recv_buf, 512, 0);
        LOGI("Admin TCP recv len: %d", recv_len);
        LWPBASE* base = (LWPBASE*)recv_buf;
        if (base->type == LPGP_LWPCREATEBATTLE && base->size == sizeof(LWPCREATEBATTLE)) {
            LOGI("LWPCREATEBATTLE received");
            LWPCREATEBATTLEOK reply_p;
            LWPCREATEBATTLE* p = (LWPCREATEBATTLE*)base;
            LWPUCKGAME* puck_game = new_puck_game(server->update_frequency, (LW_PUCK_GAME_MAP)p->GameMap);
            memcpy(puck_game->id1, p->Id1, sizeof(puck_game->id1));
            memcpy(puck_game->id2, p->Id2, sizeof(puck_game->id2));
            memcpy(puck_game->nickname[0], p->Nickname1, sizeof(puck_game->nickname));
            memcpy(puck_game->target_nickname[0], p->Nickname2, sizeof(puck_game->target_nickname));
            const int battle_id = server->battle_counter + 1; // battle id is 1-based index
            LOGI("LWPCREATEBATTLE: Create a new puck game instance '%s' vs '%s' (battle id = %d) (bot = %d)",
                 p->Nickname1, p->Nickname2, battle_id, p->BotBattle);
            puck_game->bogus_opponent = p->BotBattle;
            puck_game->server = server;
            puck_game->battle_id = battle_id;
            puck_game->c1_token = pcg32_random();
            puck_game->c2_token = pcg32_random();
            // record data init
            puck_game->record_replay_mode = 0;
            if (puck_game->record_replay_mode) {
                size_t file_size;
                char* replay_data = server_create_binary_from_file(puck_game->game_map == LPGM_OCTAGON ? "2018_06_18_19_02_24-00003.dat" : "2018_06_18_19_01_35-00001.dat", &file_size);
                LWPUCKGAMERECORD* record = malloc(sizeof(LWPUCKGAMERECORD));
                int decompressed_bytes = LZ4_decompress_safe(replay_data, (char*)record, (int)file_size, sizeof(LWPUCKGAMERECORD));
                if (decompressed_bytes > 0) {
                    puck_game->record = record;
                } else {
                    free(puck_game->record);
                    puck_game->record = 0;
                    LOGEP("decompression of replay data failed: decompressed_bytes == %d", decompressed_bytes);
                }
            } else {
                puck_game->record = calloc(1, sizeof(LWPUCKGAMERECORD));
                puck_game->record->game_map = puck_game->game_map;
                memcpy(puck_game->record->nickname, puck_game->nickname, LWMIN(sizeof(puck_game->record->nickname), sizeof(puck_game->nickname)));
                memcpy(puck_game->record->target_nickname, puck_game->target_nickname, LWMIN(sizeof(puck_game->record->target_nickname), sizeof(puck_game->target_nickname)));
                memcpy(puck_game->record->id1, puck_game->id1, LWMIN(sizeof(puck_game->record->id1), sizeof(puck_game->id1)));
                memcpy(puck_game->record->id2, puck_game->id2, LWMIN(sizeof(puck_game->record->id2), sizeof(puck_game->id2)));
                memcpy(puck_game->record->id3, puck_game->id3, LWMIN(sizeof(puck_game->record->id3), sizeof(puck_game->id3)));
                memcpy(puck_game->record->id4, puck_game->id4, LWMIN(sizeof(puck_game->record->id4), sizeof(puck_game->id4)));
                puck_game->record->player_score = puck_game->player_score[0];
                puck_game->record->target_score = puck_game->target_score[0];
                puck_game->record->dash_interval = puck_game->dash_interval;
                puck_game->on_new_record_frame = puck_game_on_new_record_frame;
            }
            // Build reply packet
            reply_p.Type = LPGP_LWPCREATEBATTLEOK;
            reply_p.Size = sizeof(LWPCREATEBATTLEOK);
            reply_p.Battle_id = battle_id;
            reply_p.C1_token = puck_game->c1_token;
            reply_p.C2_token = puck_game->c2_token;
            // IpAddr, Port field is ignored for now...
            reply_p.IpAddr[0] = 192;
            reply_p.IpAddr[1] = 168;
            reply_p.IpAddr[2] = 0;
            reply_p.IpAddr[3] = 28;
            reply_p.Port = 10288;
            server->puck_game_pool[server->battle_counter] = puck_game;
            // send reply to client
            send(client_sock, (const char*)&reply_p, sizeof(LWPCREATEBATTLEOK), 0);
            // Increment and wrap battle counter (XXX)
            server->battle_counter++;
            if (server->battle_counter >= LW_PUCK_GAME_POOL_CAPACITY) {
                server->battle_counter = 0;
            }
        } else if (base->type == LPGP_LWPSUDDENDEATH && base->size == sizeof(LWPSUDDENDEATH)) {
            LOGI("LWPSUDDENDEATH received");
            LWPSUDDENDEATH* p = (LWPSUDDENDEATH*)base;
            if ((p->Battle_id < 1) || p->Battle_id > LW_PUCK_GAME_POOL_CAPACITY) {
                // index out of range
            } else {
                LWPUCKGAME* pg = server->puck_game_pool[p->Battle_id - 1];
                if (pg->c1_token == p->Token || pg->c2_token == p->Token) {
                    pg->pg_player[0].current_hp = 1;
                    pg->pg_target[0].current_hp = 1;
                }
            }
        } else if (base->type == LPGP_LWPCHECKBATTLEVALID && base->size == sizeof(LWPCHECKBATTLEVALID)) {
            LOGI("LWPCHECKBATTLEVALID received");
            LWPBATTLEVALID reply_p;
            reply_p.Type = LPGP_LWPBATTLEVALID;
            reply_p.Size = sizeof(LWPBATTLEVALID);
            reply_p.Valid = 0;
            LWPCHECKBATTLEVALID* p = (LWPCHECKBATTLEVALID*)base;
            if ((p->Battle_id < 1) || p->Battle_id > LW_PUCK_GAME_POOL_CAPACITY) {
                // index out of range
            } else {
                LWPUCKGAME* pg = server->puck_game_pool[p->Battle_id - 1];
                reply_p.Valid = (pg && puck_game_state_phase_finished(pg->battle_phase) == 0);
            }
            send(client_sock, (const char*)&reply_p, sizeof(LWPBATTLEVALID), 0);
        } else {
            LOGE("Admin TCP unexpected packet");
        }
        closesocket(client_sock);
    }
#if !LW_PLATFORM_WIN32
#pragma clang diagnostic pop
#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCDFAInspection"
#endif
    return 0;
#if !LW_PLATFORM_WIN32
#pragma clang diagnostic pop
#endif
}

void fill_state2_gameobject(LWPSTATE2GAMEOBJECT* packet_field,
                            const LWSERVER* server,
                            const LWPUCKGAMEOBJECT* go) {
    packet_field->pos = numcomp_compress_vec3(go->pos,
                                              &server->numcomp.v[LNVT_POS]);
    packet_field->rot = numcomp_compress_mat4x4((vec4 const*)go->rot,
                                                &server->numcomp.q[LNQT_ROT]);
    packet_field->speed = (unsigned short)numcomp_compress_float(go->speed,
                                                                 &server->numcomp.f[LNFT_PUCK_SPEED]);
    packet_field->move_rad = (unsigned short)numcomp_compress_float(numcomp_wrap_radian(go->move_rad),
                                                                    &server->numcomp.f[LNFT_PUCK_MOVE_RAD]);
}

void send_puck_game_state2(LWSERVER* server,
                           const LWPUCKGAME* puck_game,
                           int player_no,
                           struct sockaddr* sa) {
    LW_PUCK_GAME_OBJECT player_go_enum = LPGO_PLAYER;
    LW_PUCK_GAME_OBJECT target_go_enum = LPGO_TARGET;
    const LWPUCKGAMEPLAYER* player = &puck_game->pg_player[0];
    const LWPUCKGAMEPLAYER* target = &puck_game->pg_target[0];
    const int* wall_hit_bit = &puck_game->wall_hit_bit_send_buf_1;
    if (player_no == 2) {
        player = &puck_game->pg_target[0];
        target = &puck_game->pg_player[0];
        wall_hit_bit = &puck_game->wall_hit_bit_send_buf_2;
    }
    LWPSTATE2 packet_state;
    packet_state.type = LPGP_LWPSTATE2;
    packet_state.puck_reflect_size = (unsigned char)numcomp_compress_float(puck_game->puck_reflect_size,
                                                                           &server->numcomp.f[LNFT_PUCK_REFLECT_SIZE]);
    LOGIx("puck_reflect_size = %f", puck_game->puck_reflect_size);
    packet_state.update_tick = (unsigned short)puck_game->update_tick;
    fill_state2_gameobject(&packet_state.go[0], server, &puck_game->go[LPGO_PUCK]);
    fill_state2_gameobject(&packet_state.go[1], server, &puck_game->go[player_go_enum]);
    fill_state2_gameobject(&packet_state.go[2], server, &puck_game->go[target_go_enum]);
    puck_game_fill_state_bitfield(&packet_state.bf, puck_game, player, target, wall_hit_bit);
    if (*wall_hit_bit && player_no == 1) {
        LOGIx("WALL HIT BIT: %d", *wall_hit_bit);
    }
    // send!
    double tp = lwtimepoint_now_seconds();
    sendto(server->s,
           (const char*)&packet_state,
           sizeof(packet_state),
           0,
           sa,
           server->slen);
    // log elapsed time
    double elapsed = lwtimepoint_now_seconds() - tp;
    LOGIx("Broadcast sendto elapsed: %.3f ms", elapsed * 1000);
}

void broadcast_state_packet(LWSERVER* server, const LWCONN* conn, int conn_capacity) {
    int sent = 0;
    for (int i = 0; i < conn_capacity; i++) {
        if (conn[i].ipport
            && check_player_no(conn[i].player_no)
            && conn[i].battle_id >= 1
            && conn[i].battle_id <= LW_PUCK_GAME_POOL_CAPACITY) {
            LWPUCKGAME* puck_game = server->puck_game_pool[conn[i].battle_id - 1];
            if (puck_game) {
                send_puck_game_state2(server,
                                      puck_game,
                                      conn[i].player_no,
                                      (struct sockaddr*)&conn[i].si);
                // clear wall hit bit send buf for each player
                if (conn[i].player_no == 1) {
                    puck_game->wall_hit_bit_send_buf_1 = 0;
                } else {
                    puck_game->wall_hit_bit_send_buf_2 = 0;
                }
                // sent okay
                sent = 1;
            }
        }
    }
    if (sent) {
        server->broadcast_count++;
    }
}

static int puck_game_winner(const LWPUCKGAME* puck_game) {
    if (puck_game->pg_player[0].current_hp > 0
        && puck_game->pg_target[0].current_hp > 0
        && puck_game->time < puck_game->total_time) {
        // not yet finished
        return -1;
    }
    const int hp_diff = puck_game->pg_player[0].current_hp - puck_game->pg_target[0].current_hp;
    if (hp_diff == 0) {
        return 0;
    } else if (hp_diff > 0) {
        return 1;
    } else {
        return 2;
    }
}

int tcp_send_battle_result(LWTCP* tcp,
                           int backoffMs,
                           const LWPUCKGAME* puck_game,
                           const unsigned int* id1,
                           const unsigned int* id2,
                           const char* nickname1,
                           const char* nickname2,
                           int winner,
                           int logic_hz) {
    if (tcp == 0) {
        LOGEP("tcp null");
        return -1;
    }
    NEW_TCP_PACKET_CAPITAL(LWPBATTLERESULT, p);
    p.Winner = winner;
    memcpy(p.Player[0].Id, id1, sizeof(p.Player[0].Id));
    memcpy(p.Player[1].Id, id2, sizeof(p.Player[1].Id));
    memcpy(p.Player[0].Nickname, nickname1, sizeof(p.Player[0].Nickname));
    memcpy(p.Player[1].Nickname, nickname2, sizeof(p.Player[1].Nickname));
    p.Player[0].Stat = puck_game->battle_stat[0];
    p.Player[1].Stat = puck_game->battle_stat[1];
    p.BattleTimeSec = (int)roundf(puck_game_elapsed_time(puck_game->update_tick, logic_hz));
    p.TotalHp = puck_game->pg_player[0].total_hp;
    memcpy(tcp->send_buf, &p, sizeof(p));
    int send_result = (int)send(tcp->connect_socket, tcp->send_buf, sizeof(p), 0);
    if (send_result < 0) {
        if (backoffMs > 10 * 1000 /* 10 seconds */) {
            LOGEP("tcp_send_battle_result: all retries failed!!!");
            return -1;
        } else if (backoffMs > 0) {
            LOGEP("tcp_send_battle_result: retrying in %d ms...", backoffMs);
            struct timespec ts;
            ts.tv_sec = 0;
            ts.tv_nsec = backoffMs * 1000 * 1000;
            thrd_sleep(&ts, 0);
        }
        tcp_connect(tcp);
        return tcp_send_battle_result(tcp,
                                      backoffMs * 2,
                                      puck_game,
                                      id1,
                                      id2,
                                      nickname1,
                                      nickname2,
                                      winner,
                                      logic_hz);
    }
    return send_result;
}

void process_battle_reward(LWPUCKGAME* puck_game, LWTCP* reward_service, int logic_hz) {
    tcp_send_battle_result(reward_service,
                           300,
                           puck_game,
                           puck_game->id1,
                           puck_game->id2,
                           puck_game->nickname[0],
                           puck_game->target_nickname[0],
                           puck_game_winner(puck_game),
                           logic_hz);
}
#if !LW_PLATFORM_WIN32
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#endif
void reward_service_on_connect(LWTCP* tcp, const char* path_prefix) {
    LOGI("Reward service connected. %s:%d (path_prefix:%s)",
         tcp->host_addr.host,
         tcp->host_addr.port,
         path_prefix);
}
#if !LW_PLATFORM_WIN32
#pragma clang diagnostic pop
#endif

int reward_service_on_recv_packets(LWTCP* tcp) {
    LOGI("Packet received (%d bytes) from reward service.", tcp->recv_buf_not_parsed);
    return tcp->recv_buf_not_parsed;
}

#if !LW_PLATFORM_WIN32
#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCDFAInspection"
#endif
int main(int argc, char* argv[]) {
    LOGI("LAIDOFF-SERVER: Greetings.");
    LOGI("sizeof(LWPSTATE) == %zu bytes", sizeof(LWPSTATE));
    LOGI("sizeof(LWPSTATE2) == %zu bytes", sizeof(LWPSTATE2));
    numcomp_test_all();
    while (!directory_exists("assets") && LwChangeDirectory("..")) {
    }
    // Create main server instance
    const int logic_hz = 125;
    LWSERVER* server = new_server();
    server->update_frequency = logic_hz;
    // TCP listen & reply thread (listen requests from match server)
    thrd_t admin_thread;
    thrd_create(&admin_thread, tcp_admin_server_entry, server);
    // Create reward server connection
    LWHOSTADDR reward_host_addr;
    strcpy(reward_host_addr.host, "127.0.0.1");
    strcpy(reward_host_addr.port_str, "10290");
    reward_host_addr.port = atoi(reward_host_addr.port_str);
    LWTCP* reward_service = new_tcp(0,
                                    0,
                                    &reward_host_addr,
                                    reward_service_on_connect,
                                    reward_service_on_recv_packets);
    // Create a test puck game instance (not used for battle)
    const double logic_timestep = 1.0 / logic_hz;
    const double sync_timestep = 1.0 / 60;
    LWCONN conn[LW_CONN_CAPACITY];
    double logic_elapsed_ms = 0;
    double sync_elapsed_ms = 0;
#if !LW_PLATFORM_WIN32
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wfor-loop-analysis"
#pragma clang diagnostic ignored "-Wmissing-noreturn"
#endif
    while (1) {
        const double loop_start = lwtimepoint_now_seconds();
        if (logic_elapsed_ms > 0) {
            int iter = (int)(logic_elapsed_ms / (logic_timestep * 1000));
            if (iter > 0) {
                LOGIx("iter: %d", iter);
            }
            for (int i = 0; i < iter; i++) {
                for (int j = 0; j < LW_PUCK_GAME_POOL_CAPACITY; j++) {
                    LWPUCKGAME* puck_game = server->puck_game_pool[j];
                    if (puck_game && puck_game->init_ready) {
                        if (puck_game_state_phase_finished(puck_game->battle_phase) == 0
                            && update_puck_game(server, puck_game, logic_timestep) < 0) {
                            LOGI("Battle finished. (battle id = %d)", puck_game->battle_id);
                            // update last HP stat
                            puck_game->battle_stat[0].Hp = puck_game->pg_player[0].current_hp;
                            puck_game->battle_stat[1].Hp = puck_game->pg_target[0].current_hp;
                            // process reward
                            process_battle_reward(puck_game, reward_service, logic_hz);
                            // write record
                            if (puck_game->record_replay_mode == 0) {
                                puck_game_on_finalize_record(puck_game, puck_game->record);
                            }
                        }
                    }
                }
            }
            logic_elapsed_ms = fmod(logic_elapsed_ms, (logic_timestep * 1000));
        }
        if (sync_elapsed_ms > sync_timestep * 1000) {
            sync_elapsed_ms = fmod(sync_elapsed_ms, (sync_timestep * 1000));
            // Broadcast state to clients
            broadcast_state_packet(server, conn, LW_CONN_CAPACITY);
        }

        invalidate_dead_conn(conn, LW_CONN_CAPACITY, lwtimepoint_now_seconds(), 2.0);

        LOGIx("Waiting for data...");
        fflush(stdout);

        select_server(server, conn, reward_service);

        double loop_time = lwtimepoint_now_seconds() - loop_start;
        logic_elapsed_ms += loop_time * 1000;
        sync_elapsed_ms += loop_time * 1000;
        LOGIx("Loop time: %.3f ms", loop_time * 1000);
    }
#if !LW_PLATFORM_WIN32
#pragma clang diagnostic pop
#endif
    destroy_tcp(reward_service);
	reward_service = 0;
    return 0;
}

#if !LW_PLATFORM_WIN32
#pragma clang diagnostic pop
#endif
