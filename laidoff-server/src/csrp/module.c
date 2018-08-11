#include <stdlib.h>
#include <stdio.h>
#include <node_api.h>
#include "module.h"
#include "test_srp.h"
#include "srp.h"

napi_value MyFunction(napi_env env, napi_callback_info info) {
  napi_status status;
  size_t argc = 2;
  int number = 0, number2 = 0;
  napi_value argv[2];
  status = napi_get_cb_info(env, info, &argc, argv, NULL, NULL);

  if (status != napi_ok) {
    napi_throw_error(env, NULL, "Failed to parse arguments");
  }

  status = napi_get_value_int32(env, argv[0], &number);

  if (status != napi_ok) {
    napi_throw_error(env, NULL, "Invalid number was passed as argument");
  }
  
  status = napi_get_value_int32(env, argv[1], &number2);
  
  if (status != napi_ok) {
    napi_throw_error(env, NULL, "Invalid number2 was passed as argument");
  }
  
  napi_value myNumber;
  number = number * 200 + number2;
  status = napi_create_int32(env, number, &myNumber);

  if (status != napi_ok) {
    napi_throw_error(env, NULL, "Unable to create return value");
  }

  return myNumber;
}

void finalize_verifier(napi_env env, void* finalize_data, void* finalize_hint) {
  free(finalize_data);
}

int alg = SRP_SHA1;
int ng_type = SRP_NG_1024;
const char* n_hex = NULL;
const char* g_hex = NULL;

napi_value SrpVerifierNew(napi_env env, napi_callback_info info) {
  napi_status status;
  size_t argc = 4;
  napi_value argv[4];
  struct SRPVerifier * ver; // HOST-PRIVATE
  size_t username_bufsize = 32;
  size_t username_len;
  char* username = (char*)malloc(username_bufsize);
  unsigned char* bytes_s;
  size_t len_s;
  unsigned char* bytes_v;
  size_t len_v;
  unsigned char* bytes_A;
  size_t len_A;
  unsigned char* bytes_B; // output
  int len_B; // output
  void* B_result_data;
  napi_value B;
  napi_value verifier;
  napi_value ret;

  status = napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
  if (status != napi_ok) {
    napi_throw_error(env, NULL, "Failed to parse arguments");
  }

  status = napi_get_value_string_utf8(env, argv[0], username, username_bufsize, &username_len);
  if (status != napi_ok) {
    free(username);
    napi_throw_error(env, NULL, "Invalid 'username' was passed as argument");
  }
  printf("username='%s'\n", username);

  status = napi_get_buffer_info(env, argv[1], (void**)&bytes_s, &len_s);
  if (status != napi_ok) {
    napi_throw_error(env, NULL, "Invalid 's' was passed as argument");
  }
  printf("len_s=%zu\n", len_s);

  status = napi_get_buffer_info(env, argv[2], (void**)&bytes_v, &len_v);
  if (status != napi_ok) {
    napi_throw_error(env, NULL, "Invalid 'v' was passed as argument");
  }
  printf("len_v=%zu\n", len_v);

  status = napi_get_buffer_info(env, argv[3], (void**)&bytes_A, &len_A);
  if (status != napi_ok) {
    napi_throw_error(env, NULL, "Invalid 'A' was passed as argument");
  }
  printf("len_A=%zu\n", len_A);

  ver = srp_verifier_new(alg, ng_type, username,
                         (const unsigned char*)bytes_s, (int)len_s,
                         (const unsigned char*)bytes_v, (int)len_v,
                         (const unsigned char*)bytes_A, (int)len_A,
                         (const unsigned char**)&bytes_B, &len_B,
                         n_hex, g_hex);
  printf("bytes_B=%d\n", bytes_B ? 1 : 0);
  printf("len_B=%d\n", len_B);
  (void)free(username), username = NULL;

  status = napi_create_buffer_copy(env, (size_t)len_B, bytes_B, &B_result_data, &B);
  if (status != napi_ok) {
    napi_throw_error(env, NULL, "Return value 'B' could not be created");
  }
  // !!! bytes_B should not be freed (owned by member variable of 'ver')

  status = napi_create_external(env, ver, finalize_verifier, NULL, &verifier);
  if (status != napi_ok) {
    napi_throw_error(env, NULL, "Return value 'verifier' could not be created"); 
  }

  status = napi_create_object(env, &ret);
  if (status != napi_ok) {
    napi_throw_error(env, NULL, "Return object could not be created"); 
  }

  status = napi_set_named_property(env, ret, "B", B);
  if (status != napi_ok) {
    napi_throw_error(env, NULL, "Return value 'B' could not be assigned"); 
  }

  status = napi_set_named_property(env, ret, "verifier", verifier);
  if (status != napi_ok) {
    napi_throw_error(env, NULL, "Return value 'verifier' could not be assigned"); 
  }

  return ret;
}

napi_value SrpVerifierVerifySession(napi_env env, napi_callback_info info) {
  napi_status status;
  size_t argc = 2;
  napi_value argv[2];
  struct SRPVerifier * ver; // HOST-PRIVATE
  unsigned char* bytes_M;
  size_t len_M;
  unsigned char* bytes_HAMK; // output
  void* HAMK_result_data;
  napi_value HAMK;
  const int len_HAMK = 512;

  status = napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
  if (status != napi_ok) {
    napi_throw_error(env, NULL, "Failed to parse arguments");
  }

  status = napi_get_value_external(env, argv[0], (void**)&ver);
  if (status != napi_ok) {
    napi_throw_error(env, NULL, "Invalid 'verifier' was passed as argument");
  }

  status = napi_get_buffer_info(env, argv[1], (void**)&bytes_M, &len_M);
  if (status != napi_ok) {
    napi_throw_error(env, NULL, "Invalid 'M' was passed as argument");
  }
  printf("len_M=%zu\n", len_M);

  srp_verifier_verify_session(ver,
                              (const unsigned char*)bytes_M,
                              (const unsigned char**)&bytes_HAMK);
  printf("bytes_HAMK=%d\n", bytes_HAMK ? 1 : 0);

  status = napi_create_buffer_copy(env, bytes_HAMK ? (size_t)len_HAMK : 0, bytes_HAMK, &HAMK_result_data, &HAMK);
  if (status != napi_ok) {
    napi_throw_error(env, NULL, "Return value 'HAMK' could not be created");
  }
  // !!! bytes_HAMK should not be freed (owned by member variable of 'ver')
  return HAMK;
}

napi_value Init(napi_env env, napi_value exports) {
  napi_status status;
  napi_value fn;
  napi_value fnVerifierNew;
  napi_value fnVerifierVerifySession;

  status = napi_create_function(env, NULL, 0, MyFunction, NULL, &fn);
  if (status != napi_ok) {
    napi_throw_error(env, NULL, "Unable to wrap native function");
  }

  status = napi_set_named_property(env, exports, "my_function", fn);
  if (status != napi_ok) {
    napi_throw_error(env, NULL, "Unable to populate exports");
  }

  status = napi_create_function(env, NULL, 0, SrpVerifierNew, NULL, &fnVerifierNew);
  if (status != napi_ok) {
    napi_throw_error(env, NULL, "Unable to wrap SrpVerifierNew function");
  }

  status = napi_set_named_property(env, exports, "VerifierNew", fnVerifierNew);
  if (status != napi_ok) {
    napi_throw_error(env, NULL, "Unable to populate exports");
  }

  status = napi_create_function(env, NULL, 0, SrpVerifierVerifySession, NULL, &fnVerifierVerifySession);
  if (status != napi_ok) {
    napi_throw_error(env, NULL, "Unable to wrap SrpVerifierVerifySession function");
  }

  status = napi_set_named_property(env, exports, "VerifierVerifySession", fnVerifierVerifySession);
  if (status != napi_ok) {
    napi_throw_error(env, NULL, "Unable to populate exports");
  }

  test_srp_main();
  
  return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)
