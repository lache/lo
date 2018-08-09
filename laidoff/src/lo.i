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

%{
#include <stdlib.h>

%}

%typemap(in, numinputs=0) const char **OUTPUT_NO_FREE (char * temp) { $1 = &temp; }
%typemap(argout) const char **OUTPUT_NO_FREE {
if ($1==0) { lua_pushnil(L); } else { lua_pushstring(L, *$1); } SWIG_arg++;
}

%typemap(in, numinputs=0) const char **OUTPUT (char * temp) { $1 = &temp; }
%typemap(argout) const char **OUTPUT {
if ($1==0) { lua_pushnil(L); } else { lua_pushstring(L, *$1); } SWIG_arg++;
}

%typemap(freearg) const char **OUTPUT {
// FREE STR
if (*$1) free(*$1);
}

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

// srp_create_salted_verification_key
// srp_user_start_authentication
// srp_verifier_new
// write_user_data_file_string
// write_user_data_file_binary
// read_user_data_file_string
// read_user_data_file_binary
%apply SWIGTYPE** OUTPUT {const unsigned char **};
%apply const char** OUTPUT_NO_FREE {const char **username};
%apply const char** OUTPUT {const char **str};
%apply int* OUTPUT {int *};

%include "srp.h"
%include "laidoff.h"

// using the C-array
%include <carrays.i>
%array_functions(int,int)
%array_functions(float,float)
%array_functions(LWUNSIGNEDCHAR,LWUNSIGNEDCHAR)
%array_functions(LWPUCKGAMEOBJECT,PUCKGAMEOBJECT)
%array_functions(LWPUCKGAMETOWER,PUCKGAMETOWER)
