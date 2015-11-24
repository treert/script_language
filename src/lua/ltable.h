/*
** $Id: ltable.h,v 2.10.1.1 2007/12/27 13:02:25 roberto Exp $
** Lua tables (hash)
** See Copyright Notice in lua.h
*/

#ifndef ltable_h
#define ltable_h

#include "lobject.h"


#define gnode(t,i)	(&(t)->node[i])
#define gkey(n)		(&(n)->i_key.nk)
#define gval(n)		(&(n)->i_val)
#define gnext(n)	((n)->i_key.nk.next)

#define key2tval(n)	(&(n)->i_key.tvk)

//om ������ֵ��ȡֵ
LUAI_FUNC const TValue *luaH_getnum (Table *t, int key);
//om ������ֵ��ȡһ����λ��������ԭ����ֵ
LUAI_FUNC TValue *luaH_setnum (lua_State *L, Table *t, int key);
//om �ַ�����ֵ
LUAI_FUNC const TValue *luaH_getstr (Table *t, TString *key);
LUAI_FUNC TValue *luaH_setstr (lua_State *L, Table *t, TString *key);
//om �����ֵ
LUAI_FUNC const TValue *luaH_get (Table *t, const TValue *key);
LUAI_FUNC TValue *luaH_set (lua_State *L, Table *t, const TValue *key);
//om? ���캯��
LUAI_FUNC Table *luaH_new (lua_State *L, int narray, int lnhash);
//om �ı������С��hash��������hash��һ��
LUAI_FUNC void luaH_resizearray (lua_State *L, Table *t, int nasize);
LUAI_FUNC void luaH_free (lua_State *L, Table *t);
LUAI_FUNC int luaH_next (lua_State *L, Table *t, StkId key);
//om ��ȡ���鳤��
//om ʵ����ֻ��������һ��λ�ã�����t[n]!=nil and t[n+1] =nil
//om �������������
LUAI_FUNC int luaH_getn (Table *t);


#if defined(LUA_DEBUG)
LUAI_FUNC Node *luaH_mainposition (const Table *t, const TValue *key);
LUAI_FUNC int luaH_isdummy (Node *n);
#endif


#endif
