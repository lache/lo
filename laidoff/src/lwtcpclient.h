#pragma once
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _LWCONTEXT LWCONTEXT;
typedef struct _LWTCP LWTCP;
typedef struct _LWUNIQUEID LWUNIQUEID;
typedef struct _LWTTL LWTTL;

void tcp_on_connect(LWTCP* tcp, const char* path_prefix);
void tcp_ttl_on_connect(LWTCP* tcp, const char* path_prefix);
int tcp_send_queue2(LWTCP* tcp, const LWUNIQUEID* id);
int tcp_send_queue3(LWTCP* tcp, const LWUNIQUEID* id, int queue_type);
int tcp_send_suddendeath(LWTCP* tcp, int battle_id, unsigned int token);
int tcp_send_newuser(LWTCP* tcp);
int tcp_send_querynick(LWTCP* tcp, const LWUNIQUEID* id);
int tcp_send_push_token(LWTCP* tcp, int backoffMs, int domain, const char* push_token);
int tcp_send_get_leaderboard(LWTCP* tcp, int backoffMs, int start_index, int count, void (*on_leaderboard_packet)(LWCONTEXT* pLwc));
int tcp_send_get_leaderboard_reveal_player(LWTCP* tcp, int backoffMs, const LWUNIQUEID* user_id, int count);
int tcp_send_setnickname(LWTCP* tcp, const LWUNIQUEID* id, const char* nickname);
int tcp_send_cancelqueue(LWTCP* tcp, const LWUNIQUEID* id);
int tcp_send_httpget(LWTCP* tcp, const char* url, const char* headers);
int parse_recv_packets(LWTCP* tcp);
const char* lw_tcp_addr(const LWCONTEXT* pLwc);
const char* lw_tcp_port_str(const LWCONTEXT* pLwc);
int lw_tcp_port(const LWCONTEXT* pLwc);
void tcp_request_landing_page(LWTCP* tcp, const char* path_prefix, LWTTL* ttl);
#ifdef __cplusplus
}
#endif
