// lua.hpp
// Lua header files for C++
// <<extern "C">> not supplied automatically because Lua also compiles as C++

/// Spring
/// internal lua is compiled as static lib and so we don't want "extern C"
//extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
//}

