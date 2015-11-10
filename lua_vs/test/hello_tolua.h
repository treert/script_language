/*
** Lua binding: hello
** Generated automatically by tolua++-1.0.92 on 11/09/15 22:32:02.
*/

#ifndef __cplusplus
#include "stdlib.h"
#endif
#include "string.h"

#include "tolua++.h"

/* Exported function */
TOLUA_API int  tolua_hello_open (lua_State* tolua_S);

#include"hello.h"

/* function to register type */
static void tolua_reg_types (lua_State* tolua_S)
{
}

/* function: hello::show */
#ifndef TOLUA_DISABLE_tolua_hello_hello_show00
static int tolua_hello_hello_show00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isnoobj(tolua_S,1,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  {
   hello::show();
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'show'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* Open function */
TOLUA_API int tolua_hello_open (lua_State* tolua_S)
{
 tolua_open(tolua_S);
 tolua_reg_types(tolua_S);
 tolua_module(tolua_S,NULL,0);
 tolua_beginmodule(tolua_S,NULL);
  tolua_module(tolua_S,"hello",0);
  tolua_beginmodule(tolua_S,"hello");
   tolua_function(tolua_S,"show",tolua_hello_hello_show00);
  tolua_endmodule(tolua_S);
 tolua_endmodule(tolua_S);
 return 1;
}


#if defined(LUA_VERSION_NUM) && LUA_VERSION_NUM >= 501
 TOLUA_API int luaopen_hello (lua_State* tolua_S) {
 return tolua_hello_open(tolua_S);
};
#endif

