%module lo


/* -----------------------------------------------------------------------------
 * wchar.i
 *
 * Typemaps for the wchar_t type
 * These are mapped to a Lua string and are passed around by value.
 * ----------------------------------------------------------------------------- */

// note: only support for pointer right now, not fixed length strings
// TODO: determine how long a const wchar_t* is so we can write wstr2str() 
// & do the output typemap


%begin %{
#ifdef WIN32
#pragma warning(push)
#pragma warning(disable:4244)
#endif
%}
%{
#include "constants.h"
#include "dialog.h"
#include "etc1.h"
#include "extrapolator.h"
#include "field.h"
#include "file.h"
#include "font.h"
#include "image.h"
#include "input.h"
#include "ktx.h"
#include "kvmsg.h"
#include "laidoff.h"
#include "logic.h"
#include "lwanim.h"
#include "lwatlasenum.h"
#include "lwatlassprite.h"
#include "lwattrib.h"
#include "lwbattlecommand.h"
#include "lwbattlecommandresult.h"
#include "lwbattlecreature.h"
#include "lwbattlestate.h"
#include "lwbitmapcontext.h"
#include "lwbox2dcollider.h"
#include "lwbuttoncommand.h"
#include "lwcontext.h"
#include "lwdamagetext.h"
#include "lwdeltatime.h"
#include "lwenemy.h"
#include "lwfbo.h"
#include "lwfieldobject.h"
#include "lwgamescene.h"
#include "lwgl.h"
#include "lwkeyframe.h"
#include "lwlog.h"
#include "lwmacro.h"
#include "lwpkm.h"
#include "lwprogrammedtex.h"
#include "lwshader.h"
#include "lwsimpleanim.h"
#include "lwskill.h"
#include "lwskilleffect.h"
#include "lwskinmesh.h"
#include "lwtextblock.h"
#include "lwtimepoint.h"
#include "lwtouchproc.h"
#include "lwtrail.h"
#include "lwuialign.h"
#include "lwuidim.h"
#include "lwvbo.h"
#include "lwvbotype.h"
#include "mq.h"
#include "nav.h"
#include "net.h"
#include "pcg_basic.h"
#include "platform_detection.h"
#include "playersm.h"
#include "ps.h"
#include "render_admin.h"
#include "render_battle.h"
#include "render_battle_result.h"
#include "render_dialog.h"
#include "render_fan.h"
#include "render_field.h"
#include "render_font_test.h"
#include "render_ttl.h"
#include "render_puckgame.h"
#include "render_ps.h"
#include "render_skin.h"
#include "render_solid.h"
#include "render_text_block.h"
#include "script.h"
#include "sound.h"
#include "sprite.h"
#include "sprite_data.h"
#include "sysmsg.h"
#include "tex.h"
#include "tinyobj_loader_c.h"
#include "unicode.h"
#include "vertices.h"
#include "zhelpers.h"
#include "armature.h"
#include "battle.h"
#include "battle_result.h"
#include "battlelogic.h"
#include "rmsg.h"
#include "lwparabola.h"
#include "construct.h"
#include "puckgameupdate.h"
#include "puckgame.h"
#include "lwtcp.h"
#include "lwtcpclient.h"
#include "puckgamepacket.h"
#include "htmlui.h"
#include "lwttl.h"
#include "lwlnglat.h"
#include "lwhostaddr.h"
#include "srp.h"
#include "test_srp.h"
#include "../mbedtls/include/mbedtls/mbedtls-config.h"
#include "../mbedtls/include/mbedtls/aes.h"
#include "lwudp.h"
#include "lwasf.h"
#ifdef WIN32
#pragma warning(pop)
#endif
%}

%include <typemaps.i>

%ignore s_set_id;
%ignore _LWTIMEPOINT;
%ignore BMF_CHAR;
// Fix "Warning 462: Unable to set variable of type ..."
%immutable tex_atlas_filename;
%immutable atlas_conf_filename;
%immutable atlas_first_lae;
%immutable atlas_first_alpha_lae;
%immutable SOUND_FILE;
%immutable SPRITE_DATA;
%immutable armature_filename;
// Fix "Warning 451: Setting a const char * variable may leak memory."
%immutable _LWBUTTONCOMMAND::name;
%immutable _LWCONTEXT::internal_data_path;
%immutable _LWCONTEXT::user_data_path;
%immutable _LWSHADERFILENAME::debug_shader_name;
%immutable _LWSKILL::name;
%immutable _LWSKILL::desc;
%immutable _LWTEXTBLOCK::text;
%immutable _LWVBOFILENAME::filename;


%{
#include <stdlib.h>

%}

%typemap(in, numinputs=0)   const char **OUTPUT_NO_FREE (char * temp)   %{ temp = 0; $1 = &temp; %}
%typemap(argout)            const char **OUTPUT_NO_FREE                 %{ if ($1==0) { lua_pushnil(L); } else { lua_pushstring(L, *$1); } SWIG_arg++; %}

%typemap(in, numinputs=0)   const char ** STRING_OUTPUT (char * temp)   %{ temp = 0; $1 = &temp; %}
%typemap(argout)            const char ** STRING_OUTPUT                 %{ if ($1==0) { lua_pushnil(L); } else { lua_pushstring(L, *$1); } SWIG_arg++; %}
%typemap(freearg)           const char ** STRING_OUTPUT                 %{ /* FREE STR*/ if (*$1) free(*$1); %}

// srp_create_salted_verification_key
// srp_user_start_authentication
// srp_verifier_new
// write_user_data_file_string
// write_user_data_file_binary
// read_user_data_file_string
// read_user_data_file_binary
%apply SWIGTYPE** OUTPUT {const unsigned char **};
%apply const char** OUTPUT_NO_FREE {const char **username};
%apply const char** STRING_OUTPUT {const char **str};
%apply int* OUTPUT {int *};

// srp_user_start_authentication
%typemap(in,numinputs=0) SWIGTYPE** OUTPUT, int* OUTPUT (const unsigned char ** bytes_A, int * len_A)
%{  $1 = &bytes_A;
    $2 = &len_A; %}
%typemap(argout) (const unsigned char ** bytes_A, int * len_A)
%{  SWIG_write_uchar_num_array(L,*$1,*$2); SWIG_arg++;
    /*should not call free(*$1) -- it is owned by arg1! */ %}

// srp_user_process_challenge
%typemap(in,numinputs=0) SWIGTYPE** OUTPUT, int* OUTPUT (const unsigned char ** bytes_M, int * len_M)
%{  $1 = &bytes_M;
    $2 = &len_M; %}
%typemap(argout) (const unsigned char ** bytes_M, int * len_M)
%{  SWIG_write_uchar_num_array(L,*$1,*$2); SWIG_arg++;
    /*should not call free(*$1) -- it is owned by arg1! */ %}

// srp_user_get_session_key
%typemap(argout) (struct SRPUser * usr, int * key_length)
%{  SWIG_write_uchar_num_array(L,$result,*$2); SWIG_arg++; %}
%typemap(out) const unsigned char *
%{ %} /* empty out typemap needed */

// srp_verifier_get_session_key
%typemap(argout) (struct SRPVerifier * , int * )
%{  SWIG_write_uchar_num_array(L,$result,*$2); SWIG_arg++; %}
%typemap(out) const unsigned char *
%{ %} /* empty out typemap needed */


%typemap(in,numinputs=0) SWIGTYPE** OUTPUTXXX, int INPUT (unsigned char ** b, int len_b)
%{  unsigned char* b = 0; $1 = &b; %}
%typemap(argout) (unsigned char ** b, int len_b)
%{  SWIG_write_uchar_num_array(L,*$1,$2); SWIG_arg++;
    free(*$1); %}
%apply SWIGTYPE** OUTPUTXXX {unsigned char **};

%typemap(in) (const unsigned char * b, int len_b)
%{  int $1_dim;
    $1 = ($ltype)SWIG_get_uchar_num_array_var(L,$input,&$1_dim);
	if (!$1) SWIG_fail;
    $2 = $1_dim;%}
%typemap(freearg) (const unsigned char * b, int len_b)
%{	SWIG_FREE_ARRAY($1);%}
void srp_hexify(const unsigned char * b, int len_b, const char ** str);



%typemap(in,numinputs=0)    size_t length               %{ size_t* plength = &$1; %}
%typemap(arginit)           unsigned char iv[16]        %{ $1 = 0; %}
%typemap(in)                unsigned char iv[16]        %{ $1 = (unsigned char *)SWIG_get_uchar_num_array_fixed(L,$input,16); %}
%typemap(argout)            unsigned char iv[16]        %{ SWIG_write_uchar_num_array(L,$1,16); SWIG_arg++; %}
%typemap(freearg)           unsigned char iv[16]        %{ SWIG_FREE_ARRAY($1); %}
%typemap(in)                const unsigned char *input  %{ int input_length; $1 = (unsigned char *)SWIG_get_uchar_num_array_var(L,$input,&input_length), *plength = (size_t)input_length; *poutput = (unsigned char*)malloc(input_length); %}
%typemap(freearg)           const unsigned char *input  %{ SWIG_FREE_ARRAY($1); %}
%typemap(in,numinputs=0)    unsigned char *output       %{ unsigned char** poutput = &$1; %}
%typemap(argout)            unsigned char *output       %{ SWIG_write_uchar_num_array(L,$1,input_length); SWIG_arg++; %}
%typemap(freearg)           unsigned char *output       %{ free($1); %}
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

// mbedtls_aes_setkey_enc
%typemap(in)                const unsigned char *key    %{ int input_length; $1 = (unsigned char *)SWIG_get_uchar_num_array_var(L,$input,&input_length), *pkeybits = (unsigned int)(input_length * 8); /*in: key*/ %}
%typemap(in,numinputs=0)    unsigned int keybits        %{ unsigned int* pkeybits = &$1; /*in(0): keybits*/ %}
int mbedtls_aes_setkey_enc( mbedtls_aes_context *ctx,
                            const unsigned char *key,
                            unsigned int keybits );

%typemap(in,numinputs=0) const unsigned char ** OUTPUTFREE, int* OUTPUTFREE (const unsigned char ** dat, int * dat_len)
%{  $1 = &dat;
    $2 = &dat_len; %}
%typemap(argout) (const unsigned char ** dat, int * dat_len)
%{  SWIG_write_uchar_num_array(L,*$1,*$2); SWIG_arg++;
    free(*$1); /*should call free()*/ %}
int read_user_data_file_binary(const LWCONTEXT* pLwc, const char* filename, const unsigned char** dat, int* dat_len);

void srp_unhexify(const char * str, const unsigned char ** dat, int * dat_len);

// srp_create_salted_verification_key
%typemap(in) (const unsigned char * password, int len_password)
%{  int $1_dim;
    $1 = ($ltype)SWIG_get_uchar_num_array_var(L,$input,&$1_dim);
	if (!$1) SWIG_fail;
    $2 = $1_dim;%}
%typemap(freearg) (const unsigned char * password, int len_password)
%{	SWIG_FREE_ARRAY($1);%}

void write_user_data_file_binary(const LWCONTEXT* pLwc, const char* filename,
                                 const unsigned char* password, int len_password//const unsigned char* dat, int dat_len
                                 );

%typemap(in,numinputs=0) const unsigned char ** BYTES_S, int* LEN_S (const unsigned char ** bytes_s, int * len_s)
%{  $1 = &bytes_s;
    $2 = &len_s; %}
%typemap(argout) (const unsigned char ** bytes_s, int * len_s)
%{  SWIG_write_uchar_num_array(L,*$1,*$2); SWIG_arg++;
    free(*$1); /*should call free()*/ %}
    
%typemap(in,numinputs=0) const unsigned char ** BYTES_V, int* LEN_V (const unsigned char ** bytes_v, int * len_v)
%{  $1 = &bytes_v;
    $2 = &len_v; %}
%typemap(argout) (const unsigned char ** bytes_v, int * len_v)
%{  SWIG_write_uchar_num_array(L,*$1,*$2); SWIG_arg++;
    free(*$1); /*should call free()*/ %}
void srp_create_salted_verification_key(SRP_HashAlgorithm alg,
                                        SRP_NGType ng_type, const char * username,
                                        const unsigned char * password, int len_password,
                                        const unsigned char ** bytes_s, int * len_s,
                                        const unsigned char ** bytes_v, int * len_v,
                                        const char * n_hex, const char * g_hex);

%newobject srp_user_new;
%delobject srp_user_delete;
%typemap(out) struct SRPUser * %{
    // should have metatable so have custom __gc function
    SWIG_NewPointerObj(L,$result,$descriptor,1); SWIG_arg++;
    luaL_getmetatable(L, "SRPUser");
    lua_setmetatable(L, -2);
%}
struct SRPUser * srp_user_new(SRP_HashAlgorithm alg, SRP_NGType ng_type, const char * username,
                              const unsigned char * password, int len_password,
                              const char * n_hex, const char * g_hex);
                              

%newobject srp_verifier_new;
%delobject srp_verifier_delete;
%typemap(out) struct SRPVerifier * %{
    // should have metatable so have custom __gc function
    SWIG_NewPointerObj(L,$result,$descriptor,1); SWIG_arg++;
    luaL_getmetatable(L, "SRPVerifier");
    lua_setmetatable(L, -2);
%}
struct SRPVerifier * srp_verifier_new(SRP_HashAlgorithm alg, SRP_NGType ng_type, const char * username,
                                      const unsigned char * password, int len_password, // const unsigned char * bytes_s, int len_s,
                                      const unsigned char * password, int len_password, // const unsigned char * bytes_v, int len_v,
                                      const unsigned char * password, int len_password, // const unsigned char * bytes_A, int len_A,
                                      const unsigned char ** bytes_A, int * len_A, // should not be freed! real meaning: const unsigned char ** bytes_B, int * len_B,
                                      const char * n_hex, const char * g_hex);
                                      
void srp_user_process_challenge(struct SRPUser * usr,
                                const unsigned char * password, int len_password,//const unsigned char * bytes_s, int len_s,
                                const unsigned char * password, int len_password,//const unsigned char * bytes_B, int len_B,
                                const unsigned char ** bytes_A, int * len_A // should not be freed! const unsigned char ** bytes_M, int * len_M
                                );

%typemap(in)        const unsigned char * user_M  %{ int $1_dim; $1 = ($ltype)SWIG_get_uchar_num_array_var(L,$input,&$1_dim); if (!$1) SWIG_fail; %}
%typemap(freearg)   const unsigned char * user_M  %{ SWIG_FREE_ARRAY($1); %}
%typemap(in,numinputs=0) SWIGTYPE** OUTPUT (const unsigned char ** bytes_HAMK) %{ $1 = &bytes_HAMK; %}
%typemap(argout)         const unsigned char ** bytes_HAMK %{ SWIG_write_uchar_num_array(L,*$1,SHA512_DIGEST_LENGTH); SWIG_arg++; /*should not call free(*$1) -- it is owned by arg1! */ %}
void srp_verifier_verify_session(struct SRPVerifier * ver,
                                 const unsigned char * user_M,
                                 const unsigned char ** bytes_HAMK);
void srp_user_verify_session(struct SRPUser * usr,
                             const unsigned char * user_M//const unsigned char * bytes_HAMK
                             );

%typemap(in) (const char* data, int size)
%{  int $1_dim;
    $1 = ($ltype)SWIG_get_schar_num_array_var(L,$input,&$1_dim);
	if (!$1) SWIG_fail;
    $2 = $1_dim;%}
%typemap(freearg) (const char* data, int size)
%{	SWIG_FREE_ARRAY($1);%}
void udp_send(LWUDP* udp, const char* data, int size);

%include "../glfw/deps/linmath.h"
%include "lwmacro.h"
%include "constants.h"
%include "dialog.h"
%include "extrapolator.h"
%include "field.h"
%include "file.h"
%include "font.h"
%include "input.h"
%include "ktx.h"
%include "kvmsg.h"
%include "logic.h"
%include "lwanim.h"
%include "lwatlasenum.h"
%include "lwatlassprite.h"
%include "lwattrib.h"
%include "lwbattlecommand.h"
%include "lwbattlecommandresult.h"
%include "lwbattlecreature.h"
%include "lwbattlestate.h"
%include "lwbitmapcontext.h"
%include "lwbox2dcollider.h"
%include "lwbuttoncommand.h"
%include "lwcontext.h"
%include "lwdamagetext.h"
%include "lwdeltatime.h"
%include "lwenemy.h"
%include "lwfbo.h"
%include "lwfieldobject.h"
%include "lwgamescene.h"
%include "lwgl.h"
%include "lwkeyframe.h"
%include "lwlog.h"
%include "lwpkm.h"
%include "lwprogrammedtex.h"
%include "lwshader.h"
%include "lwsimpleanim.h"
%include "lwskill.h"
%include "lwskilleffect.h"
%include "lwskinmesh.h"
%include "lwtextblock.h"
%include "lwtimepoint.h"
%include "lwtouchproc.h"
%include "lwtrail.h"
%include "lwuialign.h"
%include "lwuidim.h"
%include "lwvbo.h"
%include "lwvbotype.h"
%include "mq.h"
%include "nav.h"
%include "net.h"
%include "pcg_basic.h"
%include "platform_detection.h"
%include "playersm.h"
%include "ps.h"
%include "render_admin.h"
%include "render_battle.h"
%include "render_battle_result.h"
%include "render_dialog.h"
%include "render_fan.h"
%include "render_field.h"
%include "render_font_test.h"
%include "render_ttl.h"
%include "render_puckgame.h"
%include "render_ps.h"
%include "render_skin.h"
%include "render_solid.h"
%include "render_text_block.h"
%include "script.h"
%include "sound.h"
%include "sprite.h"
%include "sprite_data.h"
%include "sysmsg.h"
%include "tex.h"
%include "tinyobj_loader_c.h"
%include "unicode.h"
%include "vertices.h"
%include "zhelpers.h"
%include "armature.h"
%include "battle.h"
%include "battle_result.h"
%include "battlelogic.h"
%include "rmsg.h"
%include "lwparabola.h"
%include "construct.h"
%include "puckgameupdate.h"
%include "puckgame.h"
%include "lwtcp.h"
%include "lwtcpclient.h"
%include "puckgamepacket.h"
%include "htmlui.h"
%include "lwttl.h"
%include "lwlnglat.h"
%include "lwhostaddr.h"
%include "test_srp.h"
%include "../mbedtls/include/mbedtls/mbedtls-config.h"
%include "../mbedtls/include/mbedtls/aes.h"
%include "srp.h"
%include "laidoff.h"
%include "lwudp.h"

// using the C-array
%include <carrays.i>
%array_functions(int,int)
%array_functions(float,float)
%array_functions(LWUNSIGNEDCHAR,LWUNSIGNEDCHAR)
%array_functions(LWPUCKGAMEOBJECT,PUCKGAMEOBJECT)
%array_functions(LWPUCKGAMETOWER,PUCKGAMETOWER)

// supplementary SWIG functions supporting custom __gc function call
%wrapper %{
SWIGINTERN int  SWIG_Lua_class_is_own(lua_State *L)
{
  swig_lua_userdata *usr;
  assert(lua_isuserdata(L,-1));  /* just in case */
  usr=(swig_lua_userdata*)lua_touserdata(L,-1);  /* get it */
  
  return usr->own;
}
%}

LWASF* lwasf_new_from_file(const char* filename);
