%module test

%begin %{
#ifdef WIN32
#pragma warning(push)
#pragma warning(disable:4244)
#endif
%}
%{
#include "../mbedtls/include/mbedtls/mbedtls-config.h"
#include "../mbedtls/include/mbedtls/aes.h"
#ifdef WIN32
#pragma warning(pop)
#endif
%}

%include <typemaps.i>

//mbedtls_aes_crypt_cbc
//%apply SWIGTYPE* INOUT[ANY] {unsigned char iv[16]};
//%apply SWIGTYPE* OUTPUT[ANY] {unsigned char *output};
%typemap(in,numinputs=0)    size_t length               %{ size_t* plength = &$1; %}
%typemap(in)                unsigned char iv[16]        %{ $1 = (unsigned char *)SWIG_get_uchar_num_array_fixed(L,$input,16); %}
%typemap(argout)            unsigned char iv[16]        %{ SWIG_write_uchar_num_array(L,$1,16); SWIG_arg++; %}
%typemap(freearg)           unsigned char iv[16]        %{ SWIG_FREE_ARRAY($1); %}
%typemap(in)                const unsigned char *input  %{ int input_length, $1 = (unsigned char *)SWIG_get_uchar_num_array_var(L,$input,&input_length), *plength = (size_t)input_length; %}
%typemap(freearg)           const unsigned char *input  %{ SWIG_FREE_ARRAY($1); %}
%typemap(in,numinputs=0)    unsigned char *output       %{ %}
%typemap(argout)            unsigned char *output       %{ SWIG_write_uchar_num_array(L,$1,input_length); SWIG_arg++; %}
%typemap(freearg)           unsigned char *output       %{ free(*$1); %}
int mbedtls_aes_crypt_cbc( mbedtls_aes_context *ctx,
                           int mode,
                           size_t length,
                           unsigned char iv[16],
                           const unsigned char *input,
                           unsigned char *output );
%clear    size_t length;
%clear    unsigned char iv[16];
%clear    const unsigned char *input;
%clear    unsigned char *output;

int mbedtls_aes_crypt_cbc_2( mbedtls_aes_context *ctx,
                             int mode,
                             size_t length,
                             unsigned char iv[16],
                             const unsigned char *input,
                             unsigned char *output );
