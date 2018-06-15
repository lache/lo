#include "lwtcpclient.h"
#include <string.h>
#include "lwtcp.h"
#include "puckgamepacket.h"
#include "lwlog.h"
#include "lwcontext.h"
#include "puckgame.h"
#include "file.h"
#include "sysmsg.h"
#include "puckgame.h"
#include "lwudp.h"
#include "puckgameupdate.h"
#include "logic.h"
#include "htmlui.h"
#include "lwttl.h"

void tcp_on_connect(LWTCP* tcp, const char* path_prefix) {
    if (get_cached_user_id(path_prefix, &tcp->user_id) == 0) {
        LOGI("Cached user id: %08x-%08x-%08x-%08x",
                tcp->user_id.v[0], tcp->user_id.v[1], tcp->user_id.v[2], tcp->user_id.v[3]);
        tcp_send_querynick(tcp, &tcp->user_id);
    } else {
        // Request a new user to be created
        tcp_send_newuser(tcp);
    }
    request_player_reveal_leaderboard(tcp, puck_game_leaderboard_items_in_page(tcp->pLwc->viewport_aspect_ratio));
}

void tcp_request_landing_page(LWTCP* tcp, const char* path_prefix, LWTTL* ttl) {
    if (!tcp) {
        LOGEP("tcp null");
        return;
    }
    // Transport Tycoon Lee
    char landing_page_url[512] = { 0, };
    // XXX setting random seed here?
    pcg32_srandom(time(0), time(0));
    const char* user_id_str = lwttl_get_or_create_user_id(ttl, tcp->pLwc);
    sprintf(landing_page_url, "/idle?u=%s", user_id_str);
    tcp_send_httpget(tcp, landing_page_url, lwttl_http_header(ttl));
}

void tcp_ttl_on_connect(LWTCP* tcp, const char* path_prefix) {
    //tcp_request_landing_page(tcp, path_prefix);
}

int tcp_send_newuser(LWTCP* tcp) {
    LOGI("Sending LWPNEWUSER");
    NEW_TCP_PACKET(LWPNEWUSER, p);
    memcpy(tcp->send_buf, &p, sizeof(p));
    return tcp_send_sendbuf(tcp, sizeof(p));
}

int tcp_send_querynick(LWTCP* tcp, const LWUNIQUEID* id) {
    LOGI("Sending LWPQUERYNICK");
    NEW_TCP_PACKET_CAPITAL(LWPQUERYNICK, p);
    memcpy(p.Id, id->v, sizeof(LWUNIQUEID));
    memcpy(tcp->send_buf, &p, sizeof(p));
    return tcp_send_sendbuf(tcp, sizeof(p));
}

int tcp_send_queue2(LWTCP* tcp, const LWUNIQUEID* id) {
    LOGI("Sending LWPQUEUE2");
    NEW_TCP_PACKET_CAPITAL(LWPQUEUE2, p);
    memcpy(p.Id, id->v, sizeof(LWUNIQUEID));
    memcpy(tcp->send_buf, &p, sizeof(p));
    return tcp_send_sendbuf(tcp, sizeof(p));
}

int tcp_send_queue3(LWTCP* tcp, const LWUNIQUEID* id, int queue_type) {
    LOGI("Sending LWPQUEUE3");
    NEW_TCP_PACKET_CAPITAL(LWPQUEUE3, p);
    memcpy(p.Id, id->v, sizeof(LWUNIQUEID));
    p.QueueType = queue_type;
    memcpy(tcp->send_buf, &p, sizeof(p));
    return tcp_send_sendbuf(tcp, sizeof(p));
}

int tcp_send_cancelqueue(LWTCP* tcp, const LWUNIQUEID* id) {
    LOGI("Sending LWPCANCELQUEUE");
    NEW_TCP_PACKET_CAPITAL(LWPCANCELQUEUE, p);
    memcpy(p.Id, id->v, sizeof(LWUNIQUEID));
    memcpy(tcp->send_buf, &p, sizeof(p));
    return tcp_send_sendbuf(tcp, sizeof(p));
}

int tcp_send_suddendeath(LWTCP* tcp, int battle_id, unsigned int token) {
    LOGI("Sending LWPSUDDENDEATH");
    NEW_TCP_PACKET_CAPITAL(LWPSUDDENDEATH, p);
    p.Battle_id = battle_id;
    p.Token = token;
    memcpy(tcp->send_buf, &p, sizeof(p));
    return tcp_send_sendbuf(tcp, sizeof(p));
}

int tcp_send_get_leaderboard_reveal_player(LWTCP* tcp, int backoffMs, const LWUNIQUEID* user_id, int count) {
    LOGI("Sending LWPGETLEADERBOARDREVEALPLAYER");
    if (tcp == 0) {
        LOGE("tcp null");
        return -1;
    }
    NEW_TCP_PACKET_CAPITAL(LWPGETLEADERBOARDREVEALPLAYER, p);
    memcpy(p.Id, user_id, sizeof(p.Id));
    p.Count = count;
    memcpy(tcp->send_buf, &p, sizeof(p));
    tcp->on_leaderboard_packet = 0; // do nothing when reply packet received
    int send_result = (int)send(tcp->connect_socket, tcp->send_buf, sizeof(p), 0);
    if (send_result < 0) {
        LOGI("Send result error: %d", send_result);
        if (backoffMs > 10 * 1000 /* 10 seconds */) {
            LOGEP("failed");
            return -1;
        } else if (backoffMs > 0) {
#if LW_PLATFORM_WIN32
            Sleep(backoffMs);
#else
            usleep(backoffMs * 1000);
#endif
        }
        tcp_connect(tcp);
        return tcp_send_get_leaderboard_reveal_player(tcp, backoffMs * 2, user_id, count);
    }
    return send_result;
}

int tcp_send_get_leaderboard(LWTCP* tcp, int backoffMs, int start_index, int count, void (*on_leaderboard_packet)(LWCONTEXT* pLwc)) {
    LOGI("Sending LWPGETLEADERBOARD");
    if (tcp == 0) {
        LOGE("tcp null");
        return -1;
    }
    NEW_TCP_PACKET_CAPITAL(LWPGETLEADERBOARD, p);
    memcpy(p.Id, &tcp->user_id, sizeof(p.Id));
    p.Start_index = start_index;
    p.Count = count;
    memcpy(tcp->send_buf, &p, sizeof(p));
    tcp->on_leaderboard_packet = on_leaderboard_packet;
    int send_result = (int)send(tcp->connect_socket, tcp->send_buf, sizeof(p), 0);
    if (send_result < 0) {
        LOGI("Send result error: %d", send_result);
        if (backoffMs > 10 * 1000 /* 10 seconds */) {
            LOGEP("failed");
            return -1;
        } else if (backoffMs > 0) {
#if LW_PLATFORM_WIN32
            Sleep(backoffMs);
#else
            usleep(backoffMs * 1000);
#endif
        }
        tcp_connect(tcp);
        return tcp_send_get_leaderboard(tcp, backoffMs * 2, start_index, count, on_leaderboard_packet);
    }
    return send_result;
}

int tcp_send_push_token(LWTCP* tcp, int backoffMs, int domain, const char* push_token) {
    LOGI("Sending LWPPUSHTOKEN");
    if (tcp == 0) {
        LOGE("tcp null");
        return -1;
    }
    NEW_TCP_PACKET_CAPITAL(LWPPUSHTOKEN, p);
    p.Domain = domain;
    strncpy(p.Push_token, push_token, sizeof(p.Push_token) - 1);
    memcpy(p.Id, tcp->user_id.v, sizeof(p.Id));
    p.Push_token[sizeof(p.Push_token) - 1] = '\0';
    memcpy(tcp->send_buf, &p, sizeof(p));
    int send_result = (int)send(tcp->connect_socket, tcp->send_buf, sizeof(p), 0);
    if (send_result < 0) {
        LOGI("Send result error: %d", send_result);
        if (backoffMs > 10 * 1000 /* 10 seconds */) {
            LOGE("tcp_send_push_token: failed");
            return -1;
        } else if (backoffMs > 0) {
#if LW_PLATFORM_WIN32
            Sleep(backoffMs);
#else
            usleep(backoffMs * 1000);
#endif
        }
        tcp_connect(tcp);
        return tcp_send_push_token(tcp, backoffMs * 2, domain, push_token);
    }
    return send_result;
}

int tcp_send_setnickname(LWTCP* tcp, const LWUNIQUEID* id, const char* nickname) {
    LOGI("Sending LWPSETNICKNAME");
    NEW_TCP_PACKET_CAPITAL(LWPSETNICKNAME, p);
    memcpy(p.Id, id->v, sizeof(p.Id));
    memcpy(p.Nickname, nickname, sizeof(p.Nickname));
    p.Nickname[sizeof(p.Nickname) - 1] = 0;
    memcpy(tcp->send_buf, &p, sizeof(p));
    return tcp_send_sendbuf(tcp, sizeof(p));
}

int tcp_send_httpget(LWTCP* tcp, const char* url, const char* headers) {
    LOGI("Sending HTTP GET request: %s", url);
    if (tcp->html_wait) {
        // preventing overlapped requests
        LOGI("Sending LWPHTTPGET ignored (wait flag up)");
        return -1;
    }
    tcp->html_wait = 1;
    memset(tcp->send_buf, 0, sizeof(tcp->send_buf));
    const char* url_prefix = url[0] != '/' ? "/" : "";
    sprintf(tcp->send_buf,
            "GET %s%s HTTP/1.1\r\n"
            "Host: %s\r\n"
            "Connection: Keep-Alive\r\n"
            "%s"
            "\r\n",
            url_prefix,
            url,
            tcp->host_addr.host,
            headers);
    return tcp_send_sendbuf(tcp, strlen(tcp->send_buf));
}

static void debug_print_leaderboard(const LWPLEADERBOARD* p) {
    LOGIx("Count: %d", p->Count);
    LOGIx("First Item Rank: %d", p->First_item_rank);
    LOGIx("First Item Tie Count: %d", p->First_item_tie_count);
    int rank = p->First_item_rank;
    int tieCount = 1;
    for (int i = 0; i < p->Count; i++) {
        LOGIx("  rank.%d %s %d", rank, p->Nickname[i], p->Score[i]);
        if (i < p->Count - 1) {
            if (p->Score[i] == p->Score[i + 1]) {
                tieCount++;
            } else {
                if (rank == p->First_item_rank) {
                    rank += p->First_item_tie_count;
                } else {
                    rank += tieCount;
                }
                tieCount = 1;
            }
        }
    }
}

int parse_content_length(const char* html_response) {
    const char* content_length_str = strstr(html_response, "\r\nContent-Length: ");
    int content_length = -1;
    if (content_length_str) {
        content_length = atoi(content_length_str + strlen("\r\nContent-Length: "));
    }
    return content_length;
}

int parse_body_length(const char* html_response) {
    const char* body_str = strstr(html_response, "\r\n\r\n");
    if (body_str) {
        const char* body = strstr(html_response, "\r\n\r\n") + strlen("\r\n\r\n");
        return strlen(body);
    } else {
        return -1;
    }
}

void refresh_body(LWTCP* tcp, void* htmlui, const char* html_response) {
    const char* body = strstr(html_response, "\r\n\r\n") + strlen("\r\n\r\n");
    LOGIx("HTTP Packet Total: %d bytes (received but not parsed)", tcp->recv_buf_not_parsed);
    LOGIx("HTTP Packet Body: %zd bytes", strlen(body));
    memset(tcp->html_body, 0, sizeof(tcp->html_body));
    strcpy(tcp->html_body, body);
    if (strncmp(html_response + strlen("HTTP/1.1 "), "302", 3) == 0) {
        // Redirect to location specified by server
        const char* location_begin = strstr(html_response, "\r\nLocation: ") + strlen("\r\nLocation: ");
        const char* location_end = strstr(location_begin, "\r\n");
        char location[512];
        size_t copy_len = LWMIN(location_end - location_begin, ARRAY_SIZE(location) - 1);
        strncpy(location, location_begin, copy_len);
        location[copy_len] = 0;
        // overlapping prevent down
        tcp->html_wait = 0;
        // start a new request
        tcp_send_httpget(tcp, location, lwttl_http_header(tcp->pLwc->ttl));
    } else {
        // Plain GET reply
        htmlui_set_refresh_html_body(htmlui, 1);
        // overlapping prevent down
        tcp->html_wait = 0;
    }
}

int append_and_refresh_body(LWTCP* tcp, void* htmlui, char* cursor) {
    size_t prev_len = strlen(tcp->html_response);
    char* prev_body_str = strstr(tcp->html_response, "\r\n\r\n");
    int prev_body_len = -1;
    if (prev_body_str) {
        char* prev_body = prev_body_str + strlen("\r\n\r\n");
        prev_body_len = strlen(prev_body);
    }
    strncat(tcp->html_response, cursor, tcp->recv_buf_not_parsed);
    tcp->html_response[prev_len + tcp->recv_buf_not_parsed] = 0; // null terminate
    int content_length = parse_content_length(tcp->html_response);
    int body_length = parse_body_length(tcp->html_response);
    if (content_length != -1 && content_length <= body_length) {
        LOGIx("HTTP body_length / content_length = %d / %d", body_length, content_length);
        char* body = strstr(tcp->html_response, "\r\n\r\n") + strlen("\r\n\r\n");
        body[body_length] = 0;
        tcp->html_body_parse_start = 0;
        refresh_body(tcp, htmlui, tcp->html_response);
        if (prev_body_len < 0) {
            return tcp->recv_buf_not_parsed;
        } else {
            return body_length - prev_body_len;
        }
        
    }
    return tcp->recv_buf_not_parsed;
}

int parse_recv_packets(LWTCP* tcp) {
    LWCONTEXT* pLwc = tcp->pLwc;
    // too small for parsing
    if (tcp->recv_buf_not_parsed < 2) {
        return -1;
    }
    int parsed_bytes = 0;
    char* cursor = tcp->recv_buf;
    while (1) {
        unsigned short packet_size = *(unsigned short*)(cursor + 0);
        // HTTP packet?
        if (strncmp("HTTP/1.1", cursor, strlen("HTTP/1.1")) == 0 && tcp->html_body_parse_start == 0) {
            tcp->html_body_parse_start = 1;
            memset(tcp->html_response, 0, sizeof(tcp->html_response));
            append_and_refresh_body(tcp, pLwc->htmlui, cursor);
            return tcp->recv_buf_not_parsed;
        }
        if (tcp->html_body_parse_start == 1) {
            //strcat(tcp->html_response, cursor);
            return append_and_refresh_body(tcp, pLwc->htmlui, cursor);
        }
        // still incomplete packet
        if (packet_size == 0 || packet_size > tcp->recv_buf_not_parsed - parsed_bytes) {
            return parsed_bytes;
        }
        unsigned short packet_type = *(unsigned short*)(cursor + 2);
        if (CHECK_PACKET(packet_type, packet_size, LWPMATCHED2)) {
            LWPMATCHED2* p = (LWPMATCHED2*)cursor;
            LOGI("LWPMATCHED2 - bid:%d,map:%d", p->battle_id, p->game_map);
            pLwc->puck_game->game_map = p->game_map;
            puck_game_set_static_default_values(pLwc->puck_game);
            pLwc->puck_game->world_roll_dir *= -1;
            puck_game_roll_to_battle(pLwc->puck_game);
            pLwc->puck_game->battle_id = p->battle_id;
            pLwc->puck_game->token = p->token;
            pLwc->puck_game->player_no = p->player_no;
            pLwc->tcp->state = LUS_MATCHED;
            // make a copy of target nickname
            memcpy(pLwc->puck_game->target_nickname[0], p->target_nickname, sizeof(p->target_nickname));
            pLwc->puck_game->target_score[0] = p->target_score;
            // create UDP socket for battle sync
            pLwc->udp = new_udp();
            // set UDP server address
            pLwc->udp_host_addr.host_resolved = *(unsigned long*)p->ipaddr;
            sprintf(pLwc->udp_host_addr.host, "%d.%d.%d.%d",
                    ((int)pLwc->udp_host_addr.host_resolved >> 0) & 0xff,
                    ((int)pLwc->udp_host_addr.host_resolved >> 8) & 0xff,
                    ((int)pLwc->udp_host_addr.host_resolved >> 16) & 0xff,
                    ((int)pLwc->udp_host_addr.host_resolved >> 24) & 0xff);
            pLwc->udp_host_addr.port = p->port;
            udp_update_addr(pLwc->udp,
                            pLwc->udp_host_addr.host_resolved,
                            pLwc->udp_host_addr.port);
            memcpy(&pLwc->puck_game->matched2, p, sizeof(pLwc->puck_game->matched2));
            // Since player_no updated
            puck_game_reset_view_proj(pLwc, pLwc->puck_game);
            //show_sys_msg(pLwc->def_sys_msg, "LWPMATCHED2 received");
        } else if (CHECK_PACKET(packet_type, packet_size, LWPQUEUEOK)) {
            LOGI("LWPQUEUEOK received");
            //show_sys_msg(pLwc->def_sys_msg, "LWPQUEUEOK received");
        } else if (CHECK_PACKET(packet_type, packet_size, LWPCANCELQUEUEOK)) {
            LOGI("LWPCANCELQUEUE received");
            show_sys_msg(pLwc->def_sys_msg, "Searching aborted");
        } else if (CHECK_PACKET(packet_type, packet_size, LWPRETRYQUEUE)) {
            LOGI("LWPRETRYQUEUE received");
            //show_sys_msg(pLwc->def_sys_msg, "LWPRETRYQUEUE received");
            // Resend QUEUE2
            tcp_send_queue2(tcp, &pLwc->tcp->user_id);
        } else if (CHECK_PACKET(packet_type, packet_size, LWPRETRYQUEUE2)) {
            LOGI("LWPRETRYQUEUE2 received");
            LWPRETRYQUEUE2* p = (LWPRETRYQUEUE2*)cursor;
            // Resend QUEUE3
            tcp_send_queue3(tcp, &pLwc->tcp->user_id, p->queueType);
        } else if (CHECK_PACKET(packet_type, packet_size, LWPRETRYQUEUELATER)) {
            LOGI("LWPRETRYQUEUELATER received");
            // match server internal error...
            // queue again only if user touches retry button
            // simulate 'battle finished' state
            show_sys_msg(pLwc->def_sys_msg, "server internal error");
            pLwc->puck_game->battle_id = 99999;
            pLwc->puck_game->token = 99999;
            pLwc->puck_game_state.bf.phase = LSP_FINISHED_DRAW;
        } else if (CHECK_PACKET(packet_type, packet_size, LWPMAYBEMATCHED)) {
            LOGI("LWPMAYBEMATCHED received");
            //show_sys_msg(pLwc->def_sys_msg, "LWPMAYBEMATCHED received");
        } else if (CHECK_PACKET(packet_type, packet_size, LWPNEWUSERDATA)) {
            LOGI("LWPNEWUSERDATA received");
            LWPNEWUSERDATA* p = (LWPNEWUSERDATA*)cursor;
            save_cached_user_id(pLwc->user_data_path, (LWUNIQUEID*)p->id);
            get_cached_user_id(pLwc->user_data_path, &pLwc->tcp->user_id);
            LOGI("[NEW] Cached user nick: %s, id: %08x-%08x-%08x-%08x",
                 p->nickname,
                 pLwc->tcp->user_id.v[0],
                 pLwc->tcp->user_id.v[1],
                 pLwc->tcp->user_id.v[2],
                 pLwc->tcp->user_id.v[3]);
            memcpy(pLwc->puck_game->nickname[0], p->nickname, sizeof(char) * LW_NICKNAME_MAX_LEN);
            pLwc->puck_game->player_rank[0] = p->rank;
            pLwc->puck_game->player_score[0] = p->score;
        } else if (CHECK_PACKET(packet_type, packet_size, LWPNICK)) {
            LOGI("LWPNICK received");
            LWPNICK* p = (LWPNICK*)cursor;
            LOGI("Cached user nick: %s", p->nickname);
            memcpy(pLwc->puck_game->nickname[0], p->nickname, sizeof(char) * LW_NICKNAME_MAX_LEN);
            pLwc->puck_game->player_score[0] = p->score;
            pLwc->puck_game->player_rank[0] = p->rank;
        } else if (CHECK_PACKET(packet_type, packet_size, LWPSYSMSG)) {
            LOGI("LWPSYSMSG received");
            LWPSYSMSG* p = (LWPSYSMSG*)cursor;
            show_sys_msg(pLwc->def_sys_msg, p->message);
        } else if (CHECK_PACKET(packet_type, packet_size, LWPLEADERBOARD)) {
            LOGI("LWPLEADERBOARD received");
            LWPLEADERBOARD* p = (LWPLEADERBOARD*)cursor;
            // Cache it first
            memcpy(&pLwc->last_leaderboard, p, sizeof(LWPLEADERBOARD));
            debug_print_leaderboard(p);
            if (tcp->on_leaderboard_packet) {
                tcp->on_leaderboard_packet(pLwc);
            }
        } else if (CHECK_PACKET(packet_type, packet_size, LWPSETNICKNAMERESULT)) {
            LOGI("LWPSETNICKNAME received");
            char nicknameMsg[256];
            LWPSETNICKNAMERESULT* p = (LWPSETNICKNAMERESULT*)cursor;
            switch (p->Result) {
            case LW_SET_NICKNAME_RESULT_OK:
                strcpy(pLwc->puck_game->nickname[0], p->Nickname);
                sprintf(nicknameMsg, "Nickname changed to [%s].", p->Nickname);
                break;
            case LW_SET_NICKNAME_RESULT_TOO_SHORT:
                sprintf(nicknameMsg, "Nickname too short.");
                break;
            case LW_SET_NICKNAME_RESULT_TOO_LONG:
                sprintf(nicknameMsg, "Nickname too long.");
                break;
            case LW_SET_NICKNAME_RESULT_TOO_NOT_ALLOWED:
                sprintf(nicknameMsg, "Nickname contains not allowed characters.");
                break;
            case LW_SET_NICKNAME_RESULT_INTERNAL_ERROR:
                sprintf(nicknameMsg, "Nickname changing failed.");
                break;
            default:
                sprintf(nicknameMsg, "Nickname changing failed (unknown result).");
                break;
            }
            show_sys_msg(pLwc->def_sys_msg, nicknameMsg);
        } else {
            LOGE("Unknown TCP packet");
        }
        parsed_bytes += packet_size;
        cursor += packet_size;
    }
    return parsed_bytes;
}

const char* lw_tcp_addr(const LWCONTEXT* pLwc) {
    return pLwc->tcp_host_addr.host;
}

const char* lw_tcp_port_str(const LWCONTEXT* pLwc) {
    return pLwc->tcp_host_addr.port_str;
}

int lw_tcp_port(const LWCONTEXT* pLwc) {
    return pLwc->tcp_host_addr.port;
}
