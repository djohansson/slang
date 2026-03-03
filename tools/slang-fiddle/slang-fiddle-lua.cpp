// slang-fiddle-lua.cpp
#include "slang.h"
#if !defined(SLANG_USE_SYSTEM_LUA) || !SLANG_USE_SYSTEM_LUA
extern "C"
{
#define MAKE_LIB 1

#if SLANG_UNIX_FAMILY
#define LUA_USE_POSIX
#endif

#include "onelua.c"
}
#endif
