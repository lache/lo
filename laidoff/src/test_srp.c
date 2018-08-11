#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef WIN32
#include <WinSock2.h>
#else
#include <sys/time.h>
#endif
#include "test_srp.h"
#ifdef WIN32
#include <time.h>
#include <windows.h>

#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
#define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64
#else
#define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL
#endif

struct timezone {
    int  tz_minuteswest; /* minutes W of Greenwich */
    int  tz_dsttime;     /* type of dst correction */
};

int gettimeofday(struct timeval *tv, struct timezone *tz) {
    FILETIME ft;
    unsigned __int64 tmpres = 0;
    static int tzflag = 0;

    if (NULL != tv) {
        GetSystemTimeAsFileTime(&ft);

        tmpres |= ft.dwHighDateTime;
        tmpres <<= 32;
        tmpres |= ft.dwLowDateTime;

        tmpres /= 10;  /*convert into microseconds*/
                       /*converting file time to unix epoch*/
        tmpres -= DELTA_EPOCH_IN_MICROSECS;
        tv->tv_sec = (long)(tmpres / 1000000UL);
        tv->tv_usec = (long)(tmpres % 1000000UL);
    }

    if (NULL != tz) {
        if (!tzflag) {
            _tzset();
            tzflag++;
        }
        tz->tz_minuteswest = _timezone / 60;
        tz->tz_dsttime = _daylight;
    }

    return 0;
}
#endif

#define NITER          1

unsigned long long get_usec() {
    struct timeval t;
    gettimeofday(&t, NULL);
    return (((unsigned long long)t.tv_sec) * 1000000) + t.tv_usec;
}

const char * test_n_hex = "EEAF0AB9ADB38DD69C33F80AFA8FC5E86072618775FF3C0B9EA2314C9C256576D674DF7496"
"EA81D3383B4813D692C6E0E0D5D8E250B98BE48E495C1D6089DAD15DC7D7B46154D6B6CE8E"
"F4AD69B15D4982559B297BCF1885C529F566660E57EC68EDBC3C05726CC02FD4CBF4976EAA"
"9AFD5138FE8376435B9FC61D2FC0EB06E3";
const char * test_g_hex = "2";


int test_srp_main() {
    struct SRPVerifier * ver; // HOST-PRIVATE
    struct SRPUser     * usr; // CLIENT-PRIVATE

    const unsigned char * bytes_s = 0; // SHARED salt
    const unsigned char * bytes_v = 0; // SHARED verifier
    const unsigned char * bytes_A = 0; // SHARED
    const unsigned char * bytes_B = 0; // SHARED

    const unsigned char * bytes_M = 0; // SHARED
    const unsigned char * bytes_HAMK = 0; // SHARED

    int len_s = 0;
    int len_v = 0;
    int len_A = 0;
    int len_B = 0;
    int len_M = 0;
    int i;

    unsigned long long start;
    unsigned long long duration;

    const char * username = "g"; // SHARED
    const char * password = "\x12\x34\x56\x78\x9a\xbc\xde\xf0"; // CLIENT-PRIVATE

    const char * auth_username = 0;
    const char * n_hex = 0;
    const char * g_hex = 0;

    SRP_HashAlgorithm alg = SHARED_SRP_HASH; // SHARED
    SRP_NGType        ng_type = SHARED_SRP_NG; // SHARED

    if (ng_type == SRP_NG_CUSTOM) {
        n_hex = test_n_hex;
        g_hex = test_g_hex;
    }

    // [1] Client: Register a new account
    // INPUT: username, password
    // OUTPUT: s, v
    const unsigned char* password_cuchar = (const unsigned char *)password;
    srp_create_salted_verification_key(alg, ng_type, username,
                                       password_cuchar,
                                       strlen(password),
                                       &bytes_s, &len_s,
                                       &bytes_v, &len_v,
                                       n_hex, g_hex);
    
    // *** User -> Host: (username, s, v) ***

    start = get_usec();

    for (i = 0; i < NITER; i++) {
        printf("iter %d\n", i);
        // [2] Client: Create an account object for authentication
        // INPUT: username, password
        // OUTPUT: usr, A
        usr = srp_user_new(alg, ng_type, username,
                           password_cuchar,
                           strlen(password), n_hex, g_hex);
        srp_user_start_authentication(usr, &auth_username, &bytes_A, &len_A);

        // *** User -> Host: (username, A) ***

        // [3] Server: Create a verifier
        // INPUT: username, s, v, A
        // OUTPUT: ver, B, K(in ver)
        ver = srp_verifier_new(alg, ng_type, username, bytes_s, len_s, bytes_v, len_v,
                               bytes_A, len_A, &bytes_B, &len_B, n_hex, g_hex);
        // HOST REJECTED AUTH
        if (!bytes_B) {
            printf("Verifier SRP-6a safety check violated!\n");
            goto cleanup;
        }

        // *** Host -> User: (s, B) ***

        // [4] Client
        // INPUT: s, A(in usr), B
        // OUTPUT : M, K(in usr)
        srp_user_process_challenge(usr, bytes_s, len_s, bytes_B, len_B, &bytes_M, &len_M);
        // CLIENT REJECTED AUTH
        if (!bytes_M) {
            printf("User SRP-6a safety check violation!\n");
            goto cleanup;
        }

        // *** User -> Host: (username, M) ***

        // [5] Server
        // INPUT: ver, M
        // OUTPUT : HAMK
        srp_verifier_verify_session(ver, bytes_M, &bytes_HAMK);
        // HOST REJECTED AUTH
        if (!bytes_HAMK) {
            printf("User authentication failed!\n");
            goto cleanup;
        }

        // *** Host -> User: (HAMK) ***

        // [6] Client
        // INTPUT: usr, HAMK
        // OUTPUT : is authed ?
        srp_user_verify_session(usr, bytes_HAMK);
        // CLIENT AUTH FAILED
        if (!srp_user_is_authenticated(usr)) {
            printf("Server authentication failed!\n");
        }

    cleanup:
        srp_verifier_delete(ver);
        srp_user_delete(usr);
    }

    duration = get_usec() - start;

    printf("Usec per call: %d\n", (int)(duration / NITER));


    free((char *)bytes_s);
    free((char *)bytes_v);

    return 0;
}
