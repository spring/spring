/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

// based heavily on `System/Config/ConfigVariable.h`

#ifndef DEFINTION_TAG_H
#define DEFINTION_TAG_H

#include <assert.h>
#include <boost/utility.hpp>
#include <boost/static_assert.hpp>
#include <map>
#include <vector>
#include <sstream>
#include <string>
#include <typeinfo>
#include "Lua/LuaParser.h"
#include "System/float3.h"
#include "System/Util.h"

//example usage: DUMMYTAG(Defs, DefClass, table, customParams)
struct table {};

namespace {
	template<typename T, typename T2>
	T scale(T v, T2 a)
	{
		return v * a;
	}

	const std::string& scale(const std::string& v, float a)
	{
		return v;
	}
	
	std::ostream& operator<<(std::ostream& os, const float3& point)
	{
		return os << "[ " <<  point.x << ", " <<  point.y << ", " <<  point.z << " ]";
	}

	std::ostream& operator<<(std::ostream& os, const table& point)
	{
		return os << "\"\"";
	}
};


/**
 * @brief Untyped definition tag meta data.
 *
 * That is, meta data of a type that does not depend on the declared type
 * of the definition tag.
 */
class DefTagMetaData : public boost::noncopyable
{
public:
	typedef TypedStringConvertibleOptionalValue<std::string> OptionalString;
	typedef TypedStringConvertibleOptionalValue<int> OptionalInt;

	virtual ~DefTagMetaData() {}

	/// @brief Get the default value of this definition tag.
	virtual const StringConvertibleOptionalValue& GetDefaultValue() const = 0;

	/// @brief Get the minimum value of this definition tag.
	virtual const StringConvertibleOptionalValue& GetMinimumValue() const = 0;

	/// @brief Get the maximum value of this definition tag.
	virtual const StringConvertibleOptionalValue& GetMaximumValue() const = 0;

	/// @brief Get the scale value of this definition tag.
	virtual const StringConvertibleOptionalValue& GetScaleValue() const = 0;

	std::string GetKey() const {
		// always return the external name
		return (externalName.IsSet()) ? externalName.ToString() : key;
	}
	const std::type_info& GetTypeInfo() const { return *typeInfo; }

	const OptionalString& GetDeclarationFile() const { return declarationFile; }
	const OptionalInt&    GetDeclarationLine() const { return declarationLine; }
	const OptionalString& GetTagFunctionStr() const { return tagFunctionStr; }
	const OptionalString& GetScaleValueStr() const { return scaleValueStr; }
	const OptionalString& GetDescription() const { return description; }
	const OptionalString& GetFallbackName() const { return fallbackName; }
	const OptionalString& GetExternalName() const { return externalName; }
	         std::string  GetInternalName() const { return key; }

	static std::string GetTypeName(const std::type_info& typeInfo);

protected:
	const char* key;
	const std::type_info* typeInfo;
	OptionalString declarationFile;
	OptionalInt declarationLine;
	OptionalString externalName;
	OptionalString fallbackName;
	OptionalString description;
	OptionalString tagFunctionStr;
	OptionalString scaleValueStr;

	template<typename F> friend class DefTagBuilder;
};


/**
 * @brief Typed definition tag meta data.
 *
 * That is, meta data of the same type as the declared type
 * of the definition tag.
 */
template<typename T>
class DefTagTypedMetaData : public DefTagMetaData
{
public:
	DefTagTypedMetaData(const char* k) { key = k; typeInfo = &typeid(T); }

	T GetData(const LuaTable& lt) const {
		T defValue = defaultValue.Get();
		if (fallbackName.IsSet()) {
			defValue = lt.Get(fallbackName.Get(), defValue);
		}

		T data = lt.Get(GetKey(), defValue);
		if (scaleValue.IsSet()) {
			data = scale(data, scaleValue.Get());
		}
		if (minimumValue.IsSet()) {
			data = std::max(data, minimumValue.Get());
		}
		if (maximumValue.IsSet()) {
			data = std::min(data, maximumValue.Get());
		}
		if (tagFunctionPtr.IsSet()) {
			data = tagFunctionPtr.Get()(data);
		}

		return data;
	}

	const StringConvertibleOptionalValue& GetDefaultValue() const { return defaultValue; }
	const StringConvertibleOptionalValue& GetMinimumValue() const { return minimumValue; }
	const StringConvertibleOptionalValue& GetMaximumValue() const { return maximumValue; }
	const StringConvertibleOptionalValue& GetScaleValue()   const { return scaleValue; }

protected:
	TypedStringConvertibleOptionalValue<T> defaultValue;
	TypedStringConvertibleOptionalValue<T> minimumValue;
	TypedStringConvertibleOptionalValue<T> maximumValue;
	TypedStringConvertibleOptionalValue<float> scaleValue;

	typedef T (*TagFunc)(T x);
	TypedStringConvertibleOptionalValue<TagFunc> tagFunctionPtr;

	template<typename F> friend class DefTagBuilder;
};


/**
 * @brief Fluent interface to declare meta data of deftag
 *
 * Uses method chaining so that a definition tag can be declared like this:
 *
 * DEFTAG(Defs, DefClass, float, example)
 *   .defaultValue(6.0f)
 *   .minimumValue(1.0f)
 *   .maximumValue(10.0f)
 *   .description("This is an example");
 * 
 * DEFTAG(Defs, DefClass, float, exampleDefTagName, exampleCppVarName);
 * DEFTAG(Defs, DefClass, float, exampleDefTagName2, subclass.exampleCppVarName[0]);
 */
template<typename T>
class DefTagBuilder
{
public:
	DefTagBuilder(DefTagTypedMetaData<T>* d) : data(d) {}

	const DefTagMetaData* GetData() const { return data; }
	operator T() const { return data->GetData(); }

#define MAKE_CHAIN_METHOD(property, type) \
	DefTagBuilder& property(type const& x) { \
		data->property = x; \
		return *this; \
	}

	typedef T (*TagFunc)(T x);

	MAKE_CHAIN_METHOD(declarationFile, const char*);
	MAKE_CHAIN_METHOD(declarationLine, int);
	MAKE_CHAIN_METHOD(externalName, std::string);
	MAKE_CHAIN_METHOD(fallbackName, std::string);
	MAKE_CHAIN_METHOD(description, std::string);
	MAKE_CHAIN_METHOD(defaultValue, T);
	MAKE_CHAIN_METHOD(minimumValue, T);
	MAKE_CHAIN_METHOD(maximumValue, T);
	MAKE_CHAIN_METHOD(scaleValue, float);
	MAKE_CHAIN_METHOD(scaleValueStr, std::string);
	MAKE_CHAIN_METHOD(tagFunctionPtr, TagFunc);
	MAKE_CHAIN_METHOD(tagFunctionStr, std::string);

#undef MAKE_CHAIN_METHOD

private:
	DefTagTypedMetaData<T>* data;
};


/**
 * @brief Definition tag declaration
 *
 * The only purpose of this class is to store the meta data created by a
 * DefTagBuilder in a global map of meta data as it is assigned to
 * an instance of this class.
 */
class DefType
{
public:
	DefType(const std::string& name);

	template<typename T> DefTagBuilder<T> AddTag(const char* name) {
		DefTagTypedMetaData<T>* meta = new DefTagTypedMetaData<T>(name);
		AddMetaData(meta);
		return DefTagBuilder<T>(meta);
	}

	template<typename T> T GetTag(const std::string& name) {
		const DefTagMetaData* meta = GetMetaData(name);
	#ifdef DEBUG
		CheckType(meta, typeid(T));
	#endif
		return static_cast<const DefTagTypedMetaData<T>*>(meta)->GetData(*luaTable);
	}

	void SetLuaTable(const LuaTable& lt)
	{
		luaTable = &lt;
	}

	typedef void (*DefInitializer)(void*);
	void AddInitializer(DefInitializer init) {
		inits.push_back(init);
	}

	void Load(void* def, const LuaTable& luaTable) {
		SetLuaTable(luaTable);
		for (std::list<DefInitializer>::const_iterator it = inits.begin(); it != inits.end(); ++it) {
			(*it)(def);
		}
	}

public:
	void OutputMetaDataMap() const;
	static void OutputTagMap();

private:
	std::list<DefInitializer> inits;
	typedef std::map<std::string, const DefTagMetaData*> MetaDataMap;
	MetaDataMap map;

	const LuaTable* luaTable;

private:
	static std::map<std::string, const DefType*>& GetTypes();
	const DefTagMetaData* GetMetaData(const std::string& key);
	void AddMetaData(const DefTagMetaData* data);
	static void CheckType(const DefTagMetaData* meta, const std::type_info& want);
};


/**
 * @brief Macro Helpers
 * @see DefTagBuilder
 */
#define CONCAT_IMPL( x, y ) x##y
#define MACRO_CONCAT( x, y ) CONCAT_IMPL( x, y )

#define GETSECONDARG( x, y, ... ) y

#define DEFTAG_CNTER(Defs, DefClass, T, name, varname) \
	static void MACRO_CONCAT(fnc_,name)(void* def) { \
		T& staticCheckType = static_cast<DefClass*>(def)->varname; \
		staticCheckType = Defs.GetTag<T>(#name); \
	} \
	void MACRO_CONCAT(add_,name)() { Defs.AddInitializer(&MACRO_CONCAT(fnc_,name)); } \
	struct MACRO_CONCAT(do_once_,name) { MACRO_CONCAT(do_once_,name)() {MACRO_CONCAT(add_,name)();} }; static MACRO_CONCAT(do_once_,name) MACRO_CONCAT(do_once_,name); \
	static DefTagBuilder<T> deftag_##name = Defs.AddTag<T>(#name)

#define tagFunction(fname) \
	tagFunctionPtr(&tagFnc_##fname).tagFunctionStr(tagFncStr_##fname)

#define scaleValue(v) \
	scaleValue(v).scaleValueStr(#v)


/**
 * @brief Macros to start the method chain used to declare a deftag.
 * @see DefTagBuilder
 */
#define DEFTAG(Defs, DefClass, T, name, ...) \
	DEFTAG_CNTER(Defs, DefClass, T, name, GETSECONDARG(unused ,##__VA_ARGS__, name))

#define DUMMYTAG(Defs, T, name) \
	static DefTagBuilder<T> MACRO_CONCAT(deftag_,__LINE__) = Defs.AddTag<T>(#name)

#define TAGFUNCTION(name, T, function) \
	static T tagFnc_##name(T x) { \
		return function; \
	} \
	static const std::string tagFncStr_##name = #function;


#endif // DEFINTION_TAG_H
