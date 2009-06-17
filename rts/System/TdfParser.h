#ifndef TDFPARSER_H_INCLUDED
#define TDFPARSER_H_INCLUDED
//
//////////////////////////////////////////////////////////////////////

#include <string>
#include <vector>
#include <map>
#include <sstream>
#include "Sync/SyncedPrimitiveIO.h"
#include "Exceptions.h"
#include "float3.h"

/**
 * Used to parse TDF Config files.
 * An example of such a file is script.txt.
 */
class TdfParser
{
public:
	struct parse_error : public content_error
	{
	private:
		std::size_t line, column;
		std::string filename;
	public:
		parse_error( std::string const& line_of_error, std::size_t line, std::size_t column, std::string const& filename ) throw();
		parse_error( std::size_t line, std::size_t column, std::string const& filename ) throw();
		parse_error( std::string const& message, std::string const& line_of_error, std::size_t line, std::size_t column, std::string const& filename ) throw();
		~parse_error() throw();
		std::size_t get_line() const;
		std::size_t get_column() const;
		std::string const& get_filename() const;
	};
	struct TdfSection
	{
		TdfSection* construct_subsection(const std::string& name );
		~TdfSection();
		
		std::map<std::string, TdfSection*> sections;
		std::map<std::string, std::string> values;
		void print( std::ostream & out ) const;
		void add_name_value(const std::string& name, const std::string& value );
		template<typename T>
		void AddPair(const std::string& key, const T& value)
		{
			std::ostringstream buf;
			buf << value;
			add_name_value(key, buf.str());
		}
	};

	TdfParser() {};
	TdfParser( std::string const& filename );
	TdfParser( const char* buffer, std::size_t size );

	void print( std::ostream & out ) const;

	void LoadFile( std::string const& file );
	void LoadBuffer( const char* buffer, std::size_t size );
	virtual ~TdfParser();


	/**
	 * Retreive a specific value from the file and returns it, returns the specified default value if not found.
	 * @param defaultvalue
	 * @param location location of value.
	 * @return returns the value on success, default otherwise.
	 */
	std::string SGetValueDef(std::string const& defaultvalue, std::string const& location) const;


	/**
	 * Retreive a specific value from the file and returns it.
	 * @param value string to store value in.
	 * @param location location of value in the form "section\\section\\ ... \\name".
	 * @return returns true on success, false otherwise and error message in value.
	 */
	bool SGetValue(std::string &value, std::string const& location) const;
	template <typename T>
	bool GetValue(T& val, const std::string& location) const
	{
		std::string buf;
		if (SGetValue(buf, location))
		{
			std::istringstream stream(buf);
			stream >> val;
			return true;
		}
		else
			return false;
	};
	
	bool GetValue(bool& val, const std::string& location) const
	{
		std::string buf;
		if (SGetValue(buf, location))
		{
			int tempval;
			std::istringstream stream(buf);
			stream >> tempval;
			if (tempval == 0)
				val = false;
			else
				val = true;
			return true;
		}
		else
			return false;
	};

	/**
	 * Treat the value as a vector and fill out vec with the items.
	 * @param location location of value in the form "section\\section\\ ... \\name".
	 * @param vec reference to a vector to store items in.
	 * @return returns number of items found.
	 */
	template<typename T>
	int GetVector(std::vector<T> &vec, std::string const& location) const
	{
		std::string vecstring;
		std::stringstream stream;
		SGetValue(vecstring, location);
		stream << vecstring;

		int i=0;
		T value;
		while(stream >> value)
		{
			vec.push_back(value);
			i++;
		}

		return i;
	}

	const std::map<std::string, std::string>& GetAllValues(std::string const& location) const;
	std::vector<std::string> GetSectionList(std::string const& location) const;
	bool SectionExist(std::string const& location) const;

	template<typename T>
	void ParseArray(std::string const& value, T *array, int length) const
	{
		std::stringstream stream;
		stream << value;

		for(int i=0; i<length; i++)
		{
			stream >> array[i];
			//char slask;
			//stream >> slask;
		}
	}

	template<typename T>
	void GetDef(T& value, const std::string& defvalue, const std::string& key) const
	{
		std::string str;
		str = SGetValueDef(defvalue, key);
		std::istringstream stream(str);
		stream >> value;
	}

	void GetDef(std::string& value, const std::string& defvalue, const std::string& key) const
	{
		value = SGetValueDef(defvalue, key);
	}

	/**
	 * Retreive a value into value, or use defvalue if it does not exist
	 * (templeted defvalue version of GetDef)
	 */
	template<typename T>
	void GetTDef(T& value, const T& defvalue, const std::string& key) const
	{
		std::string str;
		if(!SGetValue(str, key))
		{
			value = defvalue;
			return;
		}

		std::stringstream stream;
		stream << str;
		stream >> value;
	}
	
	TdfSection* GetRootSection() {return &root_section; };

private:
	TdfSection root_section;
	std::string filename;

	std::vector<std::string> GetLocationVector(std::string const& location) const;

	void parse_buffer( char const* buf, std::size_t size);

public:
	float3 GetFloat3(float3 def, std::string const& location) const;
};

#endif /* TDFPARSER_H_INCLUDED */
