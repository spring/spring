/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

// based heavily on `System/Config/ConfigVariable.h`

#ifndef DEFINTION_TAG_H
#define DEFINTION_TAG_H

#include <cassert>
#include "System/Misc/NonCopyable.h"

#include <array>
#include <vector>
#include <sstream>
#include <string>
#include <typeinfo>

#include "Lua/LuaParser.h"
#include "System/float3.h"
#include "System/SpringMath.h"

// table placeholder (used for LuaTables)
// example usage: DUMMYTAG(Defs, DefClass, table, customParams)
struct table {};

namespace {
	static inline std::ostream& operator<<(std::ostream& os, const float3& point)
	{
		return os << "[ " <<  point.x << ", " <<  point.y << ", " <<  point.z << " ]";
	}

	static inline std::ostream& operator<<(std::ostream& os, const table& t)
	{
		return os << "\"\"";
	}
}

// must be included after "std::ostream& operator<<" definitions for LLVM/Clang compilation
#include "System/StringConvertibleOptionalValue.h"


/**
 * @brief Untyped definition tag meta data.
 *
 * That is, meta data of a type that does not depend on the declared type
 * of the definition tag.
 */
class DefTagMetaData : public spring::noncopyable
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

	/// @brief returns the tag name that is read from the LuaTable.
	std::string GetKey() const {
		return (externalName.IsSet()) ? externalName.ToString() : key;
	}

	const OptionalString& GetDeclarationFile() const { return declarationFile; }
	const OptionalInt&    GetDeclarationLine() const { return declarationLine; }
	const OptionalString& GetTagFunctionStr() const { return tagFunctionStr; }
	const OptionalString& GetScaleValueStr() const { return scaleValueStr; }
	const OptionalString& GetDescription() const { return description; }
	const OptionalString& GetFallbackName() const { return fallbackName; }
	const OptionalString& GetExternalName() const { return externalName; }
	         std::string  GetInternalName() const { return key; }

	const std::type_info& GetTypeInfo() const { return *typeInfo; }
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

private:
	template<typename T1, typename T2>
	static T1 scale(T1 v, T2 a) { return v * a; }

	// does not make sense for strings
	static const std::string& scale(const std::string& v, float a) { return v; }

public:
	T GetData(const LuaTable& lt) const {
		T defValue = defaultValue.Get();
		if (fallbackName.IsSet())
			defValue = lt.Get(fallbackName.Get(), defValue);

		T data = lt.Get(GetKey(), defValue);
		if (scaleValue.IsSet())
			data = scale(data, scaleValue.Get());

		if (minimumValue.IsSet())
			data = argmax<T>(data, minimumValue.Get());
		if (maximumValue.IsSet())
			data = argmin<T>(data, maximumValue.Get());

		if (tagFunctionPtr.IsSet())
			data = tagFunctionPtr.Get()(data);

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
	DefType(const char* name);
	~DefType() {
		for (unsigned int i = 0; i < tagMetaDataCnt; i++) {
			tagMetaData[i]->~DefTagMetaData();
		}
	}

	const char* GetName() const { return name; }

	template<typename T> DefTagBuilder<T> AddTag(const char* name) {
		DefTagTypedMetaData<T>* meta = AllocTagMetaData<T>(name);
		AddTagMetaData(meta); //FIXME
		return DefTagBuilder<T>(meta);
	}

	template<typename T> T GetTag(const std::string& name) {
		const DefTagMetaData* meta = GetMetaDataByInternalKey(name);
	#ifdef DEBUG
		assert(meta != nullptr);
		CheckType(meta, typeid(T));
	#endif
		return static_cast<const DefTagTypedMetaData<T>*>(meta)->GetData(*luaTable);
	}

	typedef void (*DefInitializer)(void*);
	void AddInitializer(DefInitializer init) {
		assert(defInitFuncCnt < defInitFuncs.size());

		if (defInitFuncCnt >= defInitFuncs.size())
			return;

		defInitFuncs[defInitFuncCnt++] = init;
	}

	void Load(void* instance, const LuaTable& luaTable);
	void ReportUnknownTags(const std::string& instanceName, const LuaTable& luaTable, const std::string pre = "");

public:
	void OutputMetaDataMap() const;
	static void OutputTagMap();

private:
	std::array<DefInitializer, 1024> defInitFuncs;
	std::array<const DefTagMetaData*, 1024> tagMetaData;
	std::array<uint8_t, 1024 * 1024 * 4> metaDataMem;

	unsigned int defInitFuncCnt = 0;
	unsigned int tagMetaDataCnt = 0;
	unsigned int metaDataMemIdx = 0;

	const char* name = nullptr;
	const LuaTable* luaTable = nullptr;

private:
	static std::vector<const DefType*>& GetTypes() {
		static std::vector<const DefType*> tagtypes;
		return tagtypes;
	}

	template<typename T> DefTagTypedMetaData<T>* AllocTagMetaData(const char* name) {
		DefTagTypedMetaData<T>* tmd = nullptr;

		if ((metaDataMemIdx + sizeof(DefTagTypedMetaData<T>)) > metaDataMem.size()) {
			throw (std::bad_alloc());
			return tmd;
		}

		tmd = new (&metaDataMem[metaDataMemIdx]) DefTagTypedMetaData<T>(name);
		metaDataMemIdx += sizeof(DefTagTypedMetaData<T>);
		return tmd;
	}
	void AddTagMetaData(const DefTagMetaData* data);

	const DefTagMetaData* GetMetaDataByInternalKey(const std::string& key);
	const DefTagMetaData* GetMetaDataByExternalKey(const std::string& key);

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
	struct MACRO_CONCAT(do_once_,name) { \
		MACRO_CONCAT(do_once_,name)() { \
			Defs.AddInitializer(&MACRO_CONCAT(do_once_,name)::Initializer); \
		} \
		static void Initializer(void* instance) { \
			T& staticCheckType = static_cast<DefClass*>(instance)->varname; \
			staticCheckType = Defs.GetTag<T>(#name); \
		} \
	} static MACRO_CONCAT(do_once_,name); \
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
