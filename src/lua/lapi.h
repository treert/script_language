/*
** $Id: lapi.h,v 2.2.1.1 2007/12/27 13:02:25 roberto Exp $
** Auxiliary functions from Lua API
** See Copyright Notice in lua.h
*/

#ifndef lapi_h
#define lapi_h


#include "lobject.h"

//om 这个头文件十个什么意思，兼容考虑的吗
LUAI_FUNC void luaA_pushobject (lua_State *L, const TValue *o);

#endif
