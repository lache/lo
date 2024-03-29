#ifdef __cplusplus
extern "C" {;
#endif
/*
 * Secure Remote Password 6a implementation based on mbedtls.
 *
 * Copyright (c) 2015 Dieter Wimberger
 * https://github.com/dwimberger/mbedtls-csrp
 *
 * Derived from:
 * Copyright (c) 2010 Tom Cocagne. All rights reserved.
 * https://github.com/cocagne/csrp
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Tom Cocagne, Dieter Wimberger
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

 /*
  *
  * Purpose:       This is a direct implementation of the Secure Remote Password
  *                Protocol version 6a as described by
  *                http://srp.stanford.edu/design.html
  *
  * Author:        tom.cocagne@gmail.com (Tom Cocagne), Dieter Wimberger
  *
  * Dependencies:  mbedtls
  *
  * Usage:         Refer to test_srp.c for a demonstration
  *
  * Notes:
  *    This library allows multiple combinations of hashing algorithms and
  *    prime number constants. For authentication to succeed, the hash and
  *    prime number constants must match between
  *    srp_create_salted_verification_key(), srp_user_new(),
  *    and srp_verifier_new(). A recommended approach is to determine the
  *    desired level of security for an application and globally define the
  *    hash and prime number constants to the predetermined values.
  *
  *    As one might suspect, more bits means more security. As one might also
  *    suspect, more bits also means more processing time. The test_srp.c
  *    program can be easily modified to profile various combinations of
  *    hash & prime number pairings.
  */

#ifndef SRP_H
#define SRP_H

#define SHA1_DIGEST_LENGTH 20
#define SHA224_DIGEST_LENGTH 28
#define SHA256_DIGEST_LENGTH 32
#define SHA384_DIGEST_LENGTH 48
#define SHA512_DIGEST_LENGTH 64
#define BIGNUM	mbedtls_mpi
struct SRPVerifier;
struct SRPUser;

typedef enum {
    SRP_NG_512,
    SRP_NG_768,
    SRP_NG_1024,
    SRP_NG_2048,
    SRP_NG_4096,
    SRP_NG_8192,
    SRP_NG_CUSTOM
} SRP_NGType;

typedef enum {
    SRP_SHA1,
    SRP_SHA224,
    SRP_SHA256,
    SRP_SHA384,
    SRP_SHA512
} SRP_HashAlgorithm;

/* This library will automatically seed the mbedtls random number generator.
 *
 * The random data should include at least as many bits of entropy as the
 * largest hash function used by the application. So, for example, if a
 * 512-bit hash function is used, the random data requies at least 512
 * bits of entropy.
 *
 * Passing a null pointer to this function will cause this library to skip
 * seeding the random number generator.
 *
 * Notes:
 *    * This function is optional on Windows & Linux and mandatory on all
 *      other platforms.
 */
void srp_random_seed(const unsigned char * random_data, int data_length);


/* Out: bytes_s, len_s, bytes_v, len_v
 *
 * The caller is responsible for freeing the memory allocated for bytes_s and bytes_v
 *
 * The n_hex and g_hex parameters should be 0 unless SRP_NG_CUSTOM is used for ng_type.
 * If provided, they must contain ASCII text of the hexidecimal notation.
 */
void srp_create_salted_verification_key(SRP_HashAlgorithm alg,
                                        SRP_NGType ng_type, const char * username,
                                        const unsigned char * password, int len_password,
                                        const unsigned char ** bytes_s, int * len_s,
                                        const unsigned char ** bytes_v, int * len_v,
                                        const char * n_hex, const char * g_hex);


/* Out: bytes_B, len_B.
 *
 * On failure, bytes_B will be set to NULL and len_B will be set to 0
 *
 * The n_hex and g_hex parameters should be 0 unless SRP_NG_CUSTOM is used for ng_type
 */
struct SRPVerifier *  srp_verifier_new(SRP_HashAlgorithm alg, SRP_NGType ng_type, const char * username,
    const unsigned char * bytes_s, int len_s,
                                       const unsigned char * bytes_v, int len_v,
                                       const unsigned char * bytes_A, int len_A,
                                       const unsigned char ** bytes_B, int * len_B,
                                       const char * n_hex, const char * g_hex);


void                  srp_verifier_delete(struct SRPVerifier * ver);


int                   srp_verifier_is_authenticated(struct SRPVerifier * ver);


const char *          srp_verifier_get_username(struct SRPVerifier * ver);

/* key_length may be null */
const unsigned char * srp_verifier_get_session_key(struct SRPVerifier * ver, int * key_length);


int                   srp_verifier_get_session_key_length(struct SRPVerifier * ver);


/* user_M must be exactly srp_verifier_get_session_key_length() bytes in size */
void                  srp_verifier_verify_session(struct SRPVerifier * ver,
                                                  const unsigned char * user_M,
                                                  const unsigned char ** bytes_HAMK);

/*******************************************************************************/

/* The n_hex and g_hex parameters should be 0 unless SRP_NG_CUSTOM is used for ng_type */
struct SRPUser *      srp_user_new(SRP_HashAlgorithm alg, SRP_NGType ng_type, const char * username,
    const unsigned char * bytes_password, int len_password,
                                   const char * n_hex, const char * g_hex);

void                  srp_user_delete(struct SRPUser * usr);

int                   srp_user_is_authenticated(struct SRPUser * usr);


const char *          srp_user_get_username(struct SRPUser * usr);

/* key_length may be null */
const unsigned char * srp_user_get_session_key(struct SRPUser * usr, int * key_length);

int                   srp_user_get_session_key_length(struct SRPUser * usr);

/* Output: username, bytes_A, len_A */
void                  srp_user_start_authentication(struct SRPUser * usr, const char ** username,
                                                    const unsigned char ** bytes_A, int * len_A);

/* Output: bytes_M, len_M  (len_M may be null and will always be
 *                          srp_user_get_session_key_length() bytes in size) */
void                  srp_user_process_challenge(struct SRPUser * usr,
                                                 const unsigned char * bytes_s, int len_s,
                                                 const unsigned char * bytes_B, int len_B,
                                                 const unsigned char ** bytes_M, int * len_M);

/* bytes_HAMK must be exactly srp_user_get_session_key_length() bytes in size */
void                  srp_user_verify_session(struct SRPUser * usr, const unsigned char * bytes_HAMK);


typedef unsigned char LWUNSIGNEDCHAR, *LWUNSIGNEDCHARPTR;
void srp_hexify(const unsigned char * b, int len_b, const char ** str);
void srp_unhexify(const char * str, const unsigned char ** b, int * len_b);

#endif /* Include Guard */
#ifdef __cplusplus
}
#endif
