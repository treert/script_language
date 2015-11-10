#include"lua.hpp"
#include "hello.h"
#include "hello_tolua.h"

#include <stdarg.h>

#include<stdlib.h>

extern int luaopen_test (lua_State *L);


int main(int argc, char *argv[]) {
    char *file = NULL;
    if (argc == 1) {
        file = "my.lua";
    } else {
        file = argv[1];
    }

    lua_State *L = luaL_newstate();
	tolua_hello_open(L);
    luaL_openlibs(L);
	lua_pushcfunction(L, luaopen_test);
	lua_pushstring(L, "");
	lua_call(L, 1, 0);
 //   //lua_pop(L, 1);  /* remove lib 不懂 */
    
    int ret = luaL_dofile(L, file);
    if (ret != 0) {
        printf("Error occurs when calling luaL_dofile() Hint Machine 0x%x\n", ret);
        printf("Error: %s", lua_tostring(L, -1));
    } else printf("\nDOFILE SUCCESS\n");
    
    lua_close(L);
    system("pause");
    return 0;
}