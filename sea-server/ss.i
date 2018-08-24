%module ss
%begin %{
#ifdef WIN32
#pragma warning(push)
#pragma warning(disable:4244)
#endif
%}
%{
#include "adminmessage.h"
int post_admin_message(const unsigned char * b);
#ifdef WIN32
#pragma warning(pop)
#endif
%}

%include "adminmessage.h"

%typemap(in) (const unsigned char * b)
%{  $1 = ($ltype)(((swig_lua_userdata*)lua_touserdata(L,$input))->ptr);%}
int post_admin_message(const unsigned char * b);
