/*
** Lua binding: hello
** Generated automatically by tolua++-1.0.92 on 10/31/17 16:23:55.
*/

#ifndef __cplusplus
#include "stdlib.h"
#endif
#include "string.h"

#include "tolua++.h"

/* Exported function */
TOLUA_API int  tolua_hello_open (lua_State* tolua_S);

#include"hello.h"

/* function to release collected object via destructor */
#ifdef __cplusplus

static int tolua_collect_hello__MyClass (lua_State* tolua_S)
{
 hello::MyClass* self = (hello::MyClass*) tolua_tousertype(tolua_S,1,0);
	Mtolua_delete(self);
	return 0;
}
#endif


/* function to register type */
static void tolua_reg_types (lua_State* tolua_S)
{
 tolua_usertype(tolua_S,"hello::MyClass");
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

/* method: new of class  hello::MyClass */
#ifndef TOLUA_DISABLE_tolua_hello_hello_MyClass_new00
static int tolua_hello_hello_MyClass_new00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertable(tolua_S,1,"hello::MyClass",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  {
   hello::MyClass* tolua_ret = (hello::MyClass*)  Mtolua_new((hello::MyClass)());
    tolua_pushusertype(tolua_S,(void*)tolua_ret,"hello::MyClass");
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'new'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: new_local of class  hello::MyClass */
#ifndef TOLUA_DISABLE_tolua_hello_hello_MyClass_new00_local
static int tolua_hello_hello_MyClass_new00_local(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertable(tolua_S,1,"hello::MyClass",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  {
   hello::MyClass* tolua_ret = (hello::MyClass*)  Mtolua_new((hello::MyClass)());
    tolua_pushusertype(tolua_S,(void*)tolua_ret,"hello::MyClass");
    tolua_register_gc(tolua_S,lua_gettop(tolua_S));
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'new'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: delete of class  hello::MyClass */
#ifndef TOLUA_DISABLE_tolua_hello_hello_MyClass_delete00
static int tolua_hello_hello_MyClass_delete00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"hello::MyClass",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  hello::MyClass* self = (hello::MyClass*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'delete'", NULL);
#endif
  Mtolua_delete(self);
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'delete'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: DoSomething of class  hello::MyClass */
#ifndef TOLUA_DISABLE_tolua_hello_hello_MyClass_DoSomething00
static int tolua_hello_hello_MyClass_DoSomething00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"hello::MyClass",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  hello::MyClass* self = (hello::MyClass*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'DoSomething'", NULL);
#endif
  {
   self->DoSomething();
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'DoSomething'.",&tolua_err);
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
   #ifdef __cplusplus
   tolua_cclass(tolua_S,"MyClass","hello::MyClass","",tolua_collect_hello__MyClass);
   #else
   tolua_cclass(tolua_S,"MyClass","hello::MyClass","",NULL);
   #endif
   tolua_beginmodule(tolua_S,"MyClass");
    tolua_function(tolua_S,"new",tolua_hello_hello_MyClass_new00);
    tolua_function(tolua_S,"new_local",tolua_hello_hello_MyClass_new00_local);
    tolua_function(tolua_S,".call",tolua_hello_hello_MyClass_new00_local);
    tolua_function(tolua_S,"delete",tolua_hello_hello_MyClass_delete00);
    tolua_function(tolua_S,"DoSomething",tolua_hello_hello_MyClass_DoSomething00);
   tolua_endmodule(tolua_S);
  tolua_endmodule(tolua_S);
 tolua_endmodule(tolua_S);
 return 1;
}


#if defined(LUA_VERSION_NUM) && LUA_VERSION_NUM >= 501
 TOLUA_API int luaopen_hello (lua_State* tolua_S) {
 return tolua_hello_open(tolua_S);
};
#endif

