/*
** $Id: lobject.c,v 1.97 2003/04/03 13:35:34 roberto Exp $
** Some generic functions over Lua objects
** See Copyright Notice in lua.h
*/

#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#define lobject_c

#include "lua.h"

#include "ldo.h"
#include "lmem.h"
#include "lobject.h"
#include "lstate.h"
#include "lstring.h"
#include "lvm.h"


/* function to convert a string to a lua_Number */
#ifndef lua_str2number
#define lua_str2number(s,p)     strtod((s), (p))
#endif


const TObject luaO_nilobject = {LUA_TNIL, {NULL}};


/*
** converts an integer to a "floating point byte", represented as
** (mmmmmxxx), where the real value is (xxx) * 2^(mmmmm)
*/
int luaO_int2fb (unsigned int x) {
  int m = 0;  /* mantissa */
  while (x >= (1<<3)) {
    x = (x+1) >> 1;
    m++;
  }
  return (m << 3) | cast(int, x);
}


int luaO_log2 (unsigned int x) {
  static const lu_byte log_8[255] = {
    0,
    1,1,
    2,2,2,2,
    3,3,3,3,3,3,3,3,
    4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
    5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
    6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
    6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7
  };
  if (x >= 0x00010000) {
    if (x >= 0x01000000) return log_8[((x>>24) & 0xff) - 1]+24;
    else return log_8[((x>>16) & 0xff) - 1]+16;
  }
  else {
    if (x >= 0x00000100) return log_8[((x>>8) & 0xff) - 1]+8;
    else if (x) return log_8[(x & 0xff) - 1];
    return -1;  /* special `log' for 0 */
  }
}


int luaO_rawequalObj (const TObject *t1, const TObject *t2) {
  if (ttype(t1) != ttype(t2)) return 0;
  else switch (ttype(t1)) {
    case LUA_TNIL:
      return 1;
    case LUA_TNUMBER:
      return nvalue(t1) == nvalue(t2);
    case LUA_TBOOLEAN:
      return bvalue(t1) == bvalue(t2);  /* boolean true must be 1 !! */
    case LUA_TLIGHTUSERDATA:
      return pvalue(t1) == pvalue(t2);
    default:
      lua_assert(iscollectable(t1));
      return gcvalue(t1) == gcvalue(t2);
  }
}


int luaO_str2d (const char *s, lua_Number *result) {
  char *endptr;
  lua_Number res = lua_str2number(s, &endptr);
  if (endptr == s) return 0;  /* no conversion */
  while (isspace((unsigned char)(*endptr))) endptr++;
  if (*endptr != '\0') return 0;  /* invalid trailing characters? */
  *result = res;
  return 1;
}



static void pushstr (lua_State *L, const char *str) {
  setsvalue2s(L->top, luaS_new(L, str));
  incr_top(L);
}


/* this function handles only `%d', `%c', %f, and `%s' formats */
const char *luaO_pushvfstring (lua_State *L, const char *fmt, va_list argp) {
  int n = 1;
  pushstr(L, "");
  for (;;) {
    const char *e = strchr(fmt, '%');
    if (e == NULL) break;
    setsvalue2s(L->top, luaS_newlstr(L, fmt, e-fmt));
    incr_top(L);
    switch (*(e+1)) {
      case 's':
        pushstr(L, va_arg(argp, char *));
        break;
      case 'c': {
        char buff[2];
        buff[0] = cast(char, va_arg(argp, int));
        buff[1] = '\0';
        pushstr(L, buff);
        break;
      }
      case 'd':
        setnvalue(L->top, cast(lua_Number, va_arg(argp, int)));
        incr_top(L);
        break;
      case 'f':
        setnvalue(L->top, cast(lua_Number, va_arg(argp, l_uacNumber)));
        incr_top(L);
        break;
      case '%':
        pushstr(L, "%");
        break;
      default: lua_assert(0);
    }
    n += 2;
    fmt = e+2;
  }
  pushstr(L, fmt);
  luaV_concat(L, n+1, L->top - L->base - 1);
  L->top -= n;
  return svalue(L->top - 1);
}


const char *luaO_pushfstring (lua_State *L, const char *fmt, ...) {
  const char *msg;
  va_list argp;
  va_start(argp, fmt);
  msg = luaO_pushvfstring(L, fmt, argp);
  va_end(argp);
  return msg;
}


void luaO_chunkid (char *out, const char *source, int bufflen) {
  if (*source == '=') {
    strncpy(out, source+1, bufflen);  /* remove first char */
    out[bufflen-1] = '\0';  /* ensures null termination */
  }
  else {  /* out = "source", or "...source" */
    if (*source == '@') {
      int l;
      source++;  /* skip the `@' */
      bufflen -= sizeof(" `...' ");
      l = strlen(source);
      strcpy(out, "");
      if (l>bufflen) {
        source += (l-bufflen);  /* get last part of file name */
        strcat(out, "...");
      }
      strcat(out, source);
    }
    else {  /* out = [string "string"] */
      int len = strcspn(source, "\n");  /* stop at first newline */
      bufflen -= sizeof(" [string \"...\"] ");
      if (len > bufflen) len = bufflen;
      strcpy(out, "[string \"");
      if (source[len] != '\0') {  /* must truncate? */
        strncat(out, source, len);
        strcat(out, "...");
      }
      else
        strcat(out, source);
      strcat(out, "\"]");
    }
  }
}
