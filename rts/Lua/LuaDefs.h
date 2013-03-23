/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_DEFS_H
#define LUA_DEFS_H

#include <map>
#include <string>
#include <assert.h>

enum DataType {
	INT_TYPE,
	BOOL_TYPE,
	FLOAT_TYPE,
	STRING_TYPE,
	FUNCTION_TYPE,
	READONLY_TYPE,
	ERROR_TYPE
};

struct lua_State;
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


namespace {
	template<typename T> DataType GetDataType(T) {
		const bool valid_type = false;
		assert(valid_type);
		return ERROR_TYPE;
	}
	DataType GetDataType(unsigned)    { return INT_TYPE; }
	DataType GetDataType(int)         { return INT_TYPE; }
	DataType GetDataType(bool)        { return BOOL_TYPE; }
	DataType GetDataType(float)       { return FLOAT_TYPE; }
	DataType GetDataType(const std::string&) { return STRING_TYPE; }
};

#define ADDRESS(name) ((const char *)&name)

// Requires a "start" address, use ADDRESS()
#define ADD_INT(lua, cpp) \
	paramMap[lua] = DataElement(GetDataType(cpp), ADDRESS(cpp) - start)

#define ADD_BOOL(lua, cpp) \
	paramMap[lua] = DataElement(GetDataType(cpp), ADDRESS(cpp) - start)

#define ADD_FLOAT(lua, cpp) \
	paramMap[lua] = DataElement(GetDataType(cpp), ADDRESS(cpp) - start)

#define ADD_STRING(lua, cpp) \
	paramMap[lua] = DataElement(GetDataType(cpp), ADDRESS(cpp) - start)

#define ADD_FUNCTION(lua, cpp, func) \
	paramMap[lua] = DataElement(FUNCTION_TYPE, ADDRESS(cpp) - start, func)

// keys added through this macro will generate
// (non-fatal) ERROR_TYPE warnings if indexed
#define ADD_DEPRECATED_LUADEF_KEY(lua) \
	paramMap[lua] = DataElement();

#endif // LUA_DEFS_H
