#ifndef SPRING_LUA_USER_H
#define SPRING_LUA_USER_H

extern void* spring_lua_alloc(void* ud, void* ptr, size_t osize, size_t nsize);
extern void spring_lua_alloc_get_stats(int* allocedBytes);

#endif // SPRING_LUA_USER_H
