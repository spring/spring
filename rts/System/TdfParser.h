/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef TDF_PARSER_H
#define TDF_PARSER_H

#include <string>
#include <vector>
#include <sstream>

#include "System/Sync/SyncedPrimitiveIO.h"
#include "System/Exceptions.h"
#include "System/float3.h"
#include "System/UnorderedMap.hpp"

class LuaTable;

/**
 * Used to parse TDF Config files.
 * An example of such a file is script.txt.
 */
class TdfParser
{
public:
	struct TdfSection;
	typedef spring::unordered_map<std::string, std::string> valueMap_t;
	typedef spring::unordered_map<std::string, TdfSection*> sectionsMap_t;

	struct TdfSection
	{
		~TdfSection();

		TdfSection* construct_subsection(const std::string& name);
		void print(std::ostream& out) const;
		bool remove(const std::string& key, bool caseSensitive = true);
		void add_name_value(const std::string& name, const std::string& value);

		template<typename T>
		void AddPair(const std::string& key, const T& value);

		sectionsMap_t sections;
		valueMap_t values;
	};

	TdfParser() {};
	TdfParser(std::string const& filename);
	TdfParser(const char* buffer, size_t size);
	virtual ~TdfParser();

	void print(std::ostream& out) const;

	void LoadFile(std::string const& file);
	void LoadBuffer(const char* buffer, size_t size);

	/**
	 * Retreive a specific value from the file and returns it, returns the specified default value if not found.
	 * @param defaultValue
	 * @param location location of value.
	 * @return returns the value on success, default otherwise.
	 */
	std::string SGetValueDef(std::string const& defaultValue, std::string const& location) const;

	/**
	 * Retreive a specific value from the file and returns it.
	 * @param value string to store value or error-message in.
	 * @param location location of value in the form "section\\section\\ ... \\name".
	 * @return returns true on success, false otherwise and error message in value.
	 */
	bool SGetValue(std::string &value, std::string const& location) const;

	template <typename T>
	bool GetValue(T& val, const std::string& location) const;

	bool GetValue(bool& val, const std::string& location) const;

	/**
	 * Treat the value as a vector and fill out vec with the items.
	 * @param location location of value in the form "section\\section\\ ... \\name".
	 * @param vec reference to a vector to store items in.
	 * @return returns number of items found.
	 */
	template<typename T>
	int GetVector(std::vector<T> &vec, std::string const& location) const;

	/// Returns a map with all values in section
	const valueMap_t& GetAllValues(std::string const& location) const;
	/// Returns a vector containing all section names
	std::vector<std::string> GetSectionList(std::string const& location) const;
	bool SectionExist(std::string const& location) const;

	template<typename T>
	void ParseArray(std::string const& value, T *array, int length) const;

	template<typename T>
	void GetDef(T& value, const std::string& defvalue, const std::string& key) const;

	void GetDef(std::string& value, const std::string& defvalue, const std::string& key) const
	{
		value = SGetValueDef(defvalue, key);
	}

	/**
	 * Retreive a value into value, or use defvalue if it does not exist
	 * (templeted defvalue version of GetDef)
	 */
	template<typename T>
	void GetTDef(T& value, const T& defvalue, const std::string& key) const;

	TdfSection* GetRootSection() { return &root_section; }

private:
	TdfSection root_section;
	std::string filename;

	std::vector<std::string> GetLocationVector(std::string const& location) const;

	void ParseLuaTable(const LuaTable& table, TdfSection* currentSection);
	void ParseBuffer(char const* buf, size_t size);

public:
	float3 GetFloat3(float3 def, std::string const& location) const;
};


template<typename T>
void TdfParser::TdfSection::AddPair(const std::string& key, const T& value)
{
	std::ostringstream buf;
	buf << value;
	add_name_value(key, buf.str());
}

template <typename T>
bool TdfParser::GetValue(T& val, const std::string& location) const
{
	std::string buf;
	if (SGetValue(buf, location)) {
		std::istringstream stream(buf);
		stream >> val;
		return true;
	} else {
		return false;
	}
}

template<typename T>
int TdfParser::GetVector(std::vector<T> &vec, std::string const& location) const
{
	std::string vecstring;
	std::stringstream stream;
	SGetValue(vecstring, location);
	stream << vecstring;

	int i = 0;
	T value;
	while (stream >> value) {
		vec.push_back(value);
		i++;
	}

	return i;
}

template<typename T>
void TdfParser::ParseArray(std::string const& value, T *array, int length) const
{
	std::stringstream stream;
	stream << value;

	for (size_t i = 0; i < length; i++) {
		stream >> array[i];
		//char slask;
		//stream >> slask;
	}
}

template<typename T>
void TdfParser::GetTDef(T& value, const T& defvalue, const std::string& key) const
{
	std::string str;
	if (!SGetValue(str, key)) {
		value = defvalue;
		return;
	}

	std::stringstream stream;
	stream << str;
	stream >> value;
}

template<typename T>
void TdfParser::GetDef(T& value, const std::string& defvalue, const std::string& key) const
{
	std::string str;
	str = SGetValueDef(defvalue, key);
	std::istringstream stream(str);
	stream >> value;
}


#endif /* TDF_PARSER_H */
