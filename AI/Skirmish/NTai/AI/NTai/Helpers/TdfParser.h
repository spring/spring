#ifndef TDFPARSER_H_INCLUDED
#define TDFPARSER_H_INCLUDED
//
//////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER_
	#pragma warning(disable:4786)
#endif
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <stdexcept>

namespace ntai {

	class TdfParser {
	public:
	  struct TdfSection{
		std::map<std::string, TdfSection*> sections;
		std::map<std::string, std::string> values;
		void print( std::ostream & out ) const;
		void add_name_value( std::string const& name, std::string& value );
		TdfSection* construct_subsection( std::string const& name );
		~TdfSection();
	  };

		TdfParser(Global* G);
		TdfParser(Global* G, std::string const& filename );
		TdfParser(Global* G, const char* buffer, std::size_t size );

		void print( std::ostream & out ) const;
		void LoadFile( std::string const& file );
		void LoadVirtualFile( std::string const& file );
		void LoadBuffer( const char* buffer, std::size_t size );
		virtual ~TdfParser();


		/**
			*  @param value pointer to string to store the value in.
			*  @param ... location of value, terminate with NULL.
			*  @return true on success.

		bool GetValue(std::string &value, ...);*/

		/**
			*  Retreive a specific value from the file and returns it, gives an error messagebox if value not found.
			*  @param location location of value in the form "section\\section\\ ... \\name".
			*  @return returns the value on success, undefined on failure.
			*/	
		std::string SGetValueMSG(std::string const& location);

		/**
			*  Retreive a specific value from the file and returns it, returns the specified default value if not found.
			*  @param defaultvalue
			*  @param location location of value.
			*  @return returns the value on success, default otherwise.
			*/		
		std::string SGetValueDef(std::string const& defaultvalue, std::string const& location);


		/**
			*  Retreive a specific value from the file and returns it.
			*  @param value string to store value in.
			*  @param location location of value in the form "section\\section\\ ... \\name".
			*  @return returns true on success, false otherwise and error message in value.
			*/	
		bool SGetValue(std::string &value, std::string const& location);
		const std::map<std::string, std::string> GetAllValues(std::string const& location);
		std::vector<std::string> GetSectionList(std::string const& location);
		bool SectionExist(std::string const& location);


		template<typename T>
		void ParseArray(std::string const& value, T *array, int length){
			std::stringstream stream;
			stream << value;
			for(int i=0; i<length; i++){
				stream >> array[i];
			}
		}


		template<typename T>
		void GetMsg(T& value, const std::string& key){
			std::string str;
			str = SGetValueMSG(key);
			std::stringstream stream;
			stream << str;
			stream >> value;
		}


		template<typename T>
		void GetDef(T& value, const std::string& defvalue, const std::string& key){
			std::string str;
			str = SGetValueDef(defvalue, key);
			std::stringstream stream;
			stream << str;
			stream >> value;
		}

	private:
		TdfSection root_section;
		std::string filename;
		Global* G;
		std::vector<std::string> GetLocationVector(std::string const& location);
	public:
		float3 GetFloat3(float3 def, std::string const& location);
	};
}

#endif /* TDFPARSER_H */
