/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_DEFS_H
#define LUA_DEFS_H

#include <cassert>
#include <string>

#include "System/UnorderedMap.hpp"

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
		DataElement()
		: type(ERROR_TYPE), offset(0), func(NULL), deprecated(true) {}
		DataElement(DataType t)
		: type(t), offset(0), func(NULL), deprecated(false) {}
		DataElement(DataType t, int o)
		: type(t), offset(o), func(NULL), deprecated(false) {}
		DataElement(DataType t, int o, AccessFunc f, bool d=false)
		: type(t), offset(o), func(f), deprecated(d) {}
	public:
		DataType type;
		int offset;
		AccessFunc func;
		bool deprecated;
};


typedef spring::unordered_map<std::string, DataElement> ParamMap;


namespace {
	template<typename T> DataType GetDataType(T) {
		const bool valid_type = false;
		assert(valid_type);
		return ERROR_TYPE;
	}
	template<> inline DataType GetDataType(unsigned)           { return INT_TYPE; }
	template<> inline DataType GetDataType(int)                { return INT_TYPE; }
	template<> inline DataType GetDataType(bool)               { return BOOL_TYPE; }
	template<> inline DataType GetDataType(float)              { return FLOAT_TYPE; }
	template<> inline DataType GetDataType(std::string)        { return STRING_TYPE; }
}

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

#define ADD_DEPRECATED_FUNCTION(lua, cpp, func) \
	paramMap[lua] = DataElement(FUNCTION_TYPE, ADDRESS(cpp) - start, func, true)

// keys added through this macro will generate
// (non-fatal) ERROR_TYPE warnings if indexed
#define ADD_DEPRECATED_LUADEF_KEY(lua) \
	paramMap[lua] = DataElement();




#define DECL_LOAD_HANDLER(HandlerType, handlerInst)         \
	bool HandlerType::LoadHandler() {                       \
		std::lock_guard<spring::mutex> lk(m_singleton);     \
                                                            \
		if (handlerInst != nullptr)                         \
			return (handlerInst->IsValid());                \
		if (!HandlerType::CanLoadHandler())                 \
			return false;                                   \
                                                            \
		if (!(handlerInst = new HandlerType())->IsValid())  \
			return false;                                   \
                                                            \
		return (handlerInst->CollectGarbage(true), true);   \
	}

#define DECL_LOAD_SPLIT_HANDLER(HandlerType, handlerInst)             \
	bool HandlerType::LoadHandler(bool onlySynced) {                  \
		std::lock_guard<spring::mutex> lk(m_singleton);               \
                                                                      \
		if (handlerInst != nullptr)                                   \
			return (handlerInst->IsValid());                          \
		if (!HandlerType::CanLoadHandler())                           \
			return false;                                             \
                                                                      \
		if (!(handlerInst = new HandlerType(onlySynced))->IsValid())  \
			return false;                                             \
                                                                      \
		return (handlerInst->CollectGarbage(true), true);             \
	}

#define DECL_FREE_HANDLER(HandlerType, handlerInst)      \
	bool HandlerType::FreeHandler() {                    \
		std::lock_guard<spring::mutex> lk(m_singleton);  \
                                                         \
		if (handlerInst == nullptr)                      \
			return false;                                \
                                                         \
		HandlerType* inst = handlerInst;                 \
		handlerInst = nullptr;                           \
		inst->KillLua(true);                             \
		delete inst;                                     \
		return true;                                     \
	}


#endif // LUA_DEFS_H
