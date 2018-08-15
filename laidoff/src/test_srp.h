#pragma once

#include "srp.h"

enum {
    SHARED_SRP_HASH = SRP_SHA256,
    SHARED_SRP_NG = SRP_NG_4096,
};

int test_srp_main();
