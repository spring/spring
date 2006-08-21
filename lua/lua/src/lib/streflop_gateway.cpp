/*

This file makes the streflop functions used by LUA visible from C using a C interface
There is no corresponding .h, this file was made to work with lmathlib.c only.
See lmathlib.c for details.

*/

#include "lib/streflop/streflop.h"

// stolen from lua.h
#ifndef LUA_NUMBER
typedef float lua_Number;
#else
typedef LUA_NUMBER lua_Number;
#endif

extern "C" {

lua_Number streflop_fabs(lua_Number x) {
    return streflop::fabs(x);
}

lua_Number streflop_sin(lua_Number x) {
    return streflop::sin(x);
}

lua_Number streflop_cos(lua_Number x) {
    return streflop::cos(x);
}

lua_Number streflop_tan(lua_Number x) {
    return streflop::tan(x);
}

lua_Number streflop_asin(lua_Number x) {
    return streflop::asin(x);
}

lua_Number streflop_acos(lua_Number x) {
    return streflop::acos(x);
}

lua_Number streflop_atan(lua_Number x) {
    return streflop::atan(x);
}

lua_Number streflop_atan2(lua_Number x,lua_Number y) {
    return streflop::atan2(x,y);
}

lua_Number streflop_ceil(lua_Number x) {
    return streflop::ceil(x);
}

lua_Number streflop_floor(lua_Number x) {
    return streflop::floor(x);
}

lua_Number streflop_fmod(lua_Number x,lua_Number y) {
    return streflop::fmod(x,y);
}

lua_Number streflop_sqrt(lua_Number x) {
    return streflop::sqrt(x);
}

lua_Number streflop_pow(lua_Number x,lua_Number y) {
    return streflop::pow(x,y);
}

lua_Number streflop_log(lua_Number x) {
    return streflop::log(x);
}

lua_Number streflop_log10(lua_Number x) {
    return streflop::log10(x);
}

lua_Number streflop_exp(lua_Number x) {
    return streflop::exp(x);
}

lua_Number streflop_frexp(lua_Number x, int* e) {
    return streflop::frexp(x,e);
}

lua_Number streflop_ldexp(lua_Number x, int e) {
    return streflop::ldexp(x,e);
}

static int lua_streflop_random_seed = 0;

void streflop_seed(int s) {
    streflop::RandomInit(s);
}

lua_Number streflop_random() {
    // respect the original lua code that uses rand, and rand is auto-seeded always with the same value
    if (lua_streflop_random_seed == 0) {
        lua_streflop_random_seed = 1;
        streflop::RandomInit(1);
    }
    return streflop::Random<true, false, lua_Number>(lua_Number(0.), lua_Number(1.));
}

}
