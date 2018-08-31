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
int endpoints(char*** eps, int* n_eps);
#ifdef WIN32
#pragma warning(pop)
#endif
%}


%include <typemaps.i>

%include "adminmessage.h"

%typemap(in) (const unsigned char * b)
%{  $1 = ($ltype)(((swig_lua_userdata*)lua_touserdata(L,$input))->ptr);%}
int post_admin_message(const unsigned char * b);

%typemap(in,numinputs=0) (char *** eps, int * l)
%{  char** eps;
    int l;
    $1 = &eps;
    $2 = &l; %}
%typemap(argout) (char*** eps, int* l)
%{  lua_newtable(L);
    for (int i = 0; i < l; i++) {
        lua_pushstring(L,eps[i]);
        lua_rawseti(L,-2,i+1);
    }
    SWIG_arg++;
    for (int i = 0; i < l; i++) {
        free(eps[i]);
    }
    free(eps); /*should call free()*/ %}
int endpoints(char*** eps, int* l);
