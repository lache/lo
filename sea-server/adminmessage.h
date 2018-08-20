#pragma once

struct command {
    char type;
    char padding0;
    char padding1;
    char padding2;
};

struct spawn_command {
    command _;
    int expected_db_id;
    float x;
    float y;
};

struct travel_to_command {
    command _;
    char guid[64];
    float x;
    float y;
};

struct teleport_to_command {
    command _;
    char guid[64];
    float x;
    float y;
};

struct spawn_ship_command {
    command _;
    int expected_db_id;
    float x;
    float y;
    int port1_id;
    int port2_id;
    int reply_id;
    int expect_land;
    int ship_template_id;
};

struct spawn_ship_command_reply {
    command _;
    int db_id;
    int port1_id;
    int port2_id;
    int routed;
    int reply_id;
};

struct delete_ship_command {
    command _;
    int ship_id;
};

struct spawn_port_command {
    command _;
    int expected_db_id; // DB key
    char name[64];
    int xc;
    int yc;
    int owner_id;
    int reply_id;
    int expect_land;
};

struct spawn_port_command_reply {
    command _;
    int db_id; // DB key
    int reply_id;
    int existing;
    int too_close;
};

struct delete_port_command {
    command _;
    int port_id;
};

struct spawn_shipyard_command {
    command _;
    int expected_db_id; // DB key
    char name[64];
    int xc;
    int yc;
    int owner_id;
    int reply_id;
};

struct spawn_shipyard_command_reply {
    command _;
    int db_id; // DB key
    int reply_id;
    int existing;
};

struct delete_shipyard_command {
    command _;
    int shipyard_id;
};

struct query_nearest_shipyard_for_ship_command {
    command _;
    int ship_id;
    int reply_id;
};

struct query_nearest_shipyard_for_ship_command_reply {
    command _;
    int reply_id;
    int ship_id;
    int shipyard_id;
};

struct register_shared_secret_session_key_command {
    command _;
    int reply_id;
    char account_id[32];
    char key_str[64]; // hex-string(0x??) 64 characters --- (32-byte) "NOT NULL TERMINATED"
    int key_str_len;
};

struct register_shared_secret_session_key_command_reply {
    command _;
    int reply_id;
};
