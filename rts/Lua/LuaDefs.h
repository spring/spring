/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_DEFS_H
#define LUA_DEFS_H

#include <map>
#include <string>


enum DataType {
	INT_TYPE,
	BOOL_TYPE,
	FLOAT_TYPE,
	STRING_TYPE,
	FUNCTION_TYPE,
	READONLY_TYPE,
	ERROR_TYPE
};


typedef int (*AccessFunc)(lua_State* L, const void* data);


struct DataElement {
	public:
		DataElement() : type(ERROR_TYPE), offset(0), func(NULL) {}
		DataElement(DataType t)
		: type(t), offset(0), func(NULL) {}
		DataElement(DataType t, int o)
		: type(t), offset(o), func(NULL) {}
		DataElement(DataType t, int o, AccessFunc f)
		: type(t), offset(o), func(f) {}
	public:
		DataType type;
		int offset;
		AccessFunc func;
};


typedef std::map<std::string, DataElement> ParamMap;


/* This is unused and does not compile on GCC 4.3 -- tvo 9/9/2007
   "error: explicit template specialization cannot have a storage class"

template<typename T> static DataType GetDataType(T) {
	const bool valid_type = false;
	assert(valid_type);
	return ERROR_TYPE;
}
template<> static DataType GetDataType(int)    { return INT_TYPE; }
template<> static DataType GetDataType(bool)   { return BOOL_TYPE; }
template<> static DataType GetDataType(float)  { return FLOAT_TYPE; }
template<> static DataType GetDataType(string) { return STRING_TYPE; }
*/


#define ADDRESS(name) ((const char *)&name)

// Requires a "start" address, use ADDRESS()
#define ADD_INT(lua, cpp) \
	paramMap[lua] = DataElement(INT_TYPE, ADDRESS(cpp) - start)

#define ADD_BOOL(lua, cpp) \
	paramMap[lua] = DataElement(BOOL_TYPE, ADDRESS(cpp) - start)

#define ADD_FLOAT(lua, cpp) \
	paramMap[lua] = DataElement(FLOAT_TYPE, ADDRESS(cpp) - start)

#define ADD_STRING(lua, cpp) \
	paramMap[lua] = DataElement(STRING_TYPE, ADDRESS(cpp) - start)

#define ADD_FUNCTION(lua, cpp, func) \
	paramMap[lua] = DataElement(FUNCTION_TYPE, ADDRESS(cpp) - start, func)




#if (LUA_VERSION_NUM < 500)
#  define LUA_OPEN_LIB(L, lib) lib(L)
#else
#  define LUA_OPEN_LIB(L, lib) \
     lua_pushcfunction((L), lib); \
     lua_pcall((L), 0, 0, 0);
#endif




#endif // LUA_DEFS_H
