#include "lua.hpp"

static int hello(lua_State *L) {
    puts("test hello");
    return 0;
}

static const luaL_Reg libs[] = {
  {"hello",   hello},
  {NULL, NULL}
};

LUALIB_API int luaopen_test(lua_State *L) {
	luaL_register(L, "test", libs);
	return 1;
}
