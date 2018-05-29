#pragma once

#include "packet.h"

namespace ss {
    enum cargo_notification_type {
        cnt_created,
        cnt_loaded,
        cnt_unloaded,
        cnt_consumed,
        cnt_converted,
    };
    struct cargo_notification {
        int xc0;
        int yc0;
        int xc1;
        int yc1;
        int amount;
        cargo_notification_type cnt;
    };
}
