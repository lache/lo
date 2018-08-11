#pragma once

#include "srp.h"

enum {
    SHARED_SRP_HASH = SRP_SHA512,
    SHARED_SRP_NG = SRP_NG_8192,
};

int test_srp_main();
