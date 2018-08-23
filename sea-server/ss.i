%module ss
%begin %{
#ifdef WIN32
#pragma warning(push)
#pragma warning(disable:4244)
#endif
%}
%{
#include "adminmessage.h"
void post_admin_message(const unsigned char * b, int len_b);
#ifdef WIN32
#pragma warning(pop)
#endif
%}

%include "adminmessage.h"


%typemap(in) (const unsigned char * b, int len_b)
%{  /*int $1_dim;
    swig_lua_userdata *usr = (swig_lua_userdata*)lua_touserdata(L,$input);*/
    //$1 = ($ltype)SWIG_get_uchar_num_array_var(L,$input,&$1_dim,$type);
	//if (!$1) SWIG_fail;
    /*$2 = $1_dim;*/%}
%typemap(freearg) (const unsigned char * b, int len_b)
%{	/*SWIG_FREE_ARRAY($1);*/%}
//void post_admin_message(const unsigned char * b, int len_b);

