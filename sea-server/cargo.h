#pragma once

#include "packet.h"

namespace ss {
    struct cargo_notification {
        int xc0;
        int yc0;
        int xc1;
        int yc1;
        int amount;
        LW_TTL_CARGO_NOTIFICATION_TYPE cargo_notification_type;
    };
}
