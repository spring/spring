#ifndef KAIK_SUNPARSER_HDR
#define KAIK_SUNPARSER_HDR

#include <string>
#include <sstream>
#include <map>
#include <vector>

#include "System/float3.h"
struct AIClasses;

class CSunParser {
	public:
		CSunParser(AIClasses* ai);
		~CSunParser();

		void LoadVirtualFile(std::string filename);
		void LoadRealFile(std::string filename);

		  /**
			*  @param value pointer to string to store the value in.
			*  @param amJustHereForIntelCompilerCompatibility should always be NULL
			*  @param ... location of value, terminate with NULL.
			*  @return true on success.
			*/
		bool GetValue(std::string &value, void* amJustHereForIntelCompilerCompatibility, ...);

		  /**
			*  Retreive a specific value from the file and returns it, gives an error messagebox if value not found.
			*  @param location location of value in the form "section\\section\\ ... \\name".
			*  @return returns the value on success, undefined on failure.
			*/
		std::string SGetValueMSG(std::string location);

		  /**
			*  Retreive a specific value from the file and returns it, returns the specified default value if not found.
			*  @param defaultvalue
			*  @param location location of value.
			*  @return returns the value on success, default otherwise.
			*/
		std::string SGetValueDef(std::string defaultvalue, std::string location);

		  /**
			*  Retreive a specific value from the file and returns it.
			*  @param value string to store value in.
			*  @param location location of value in the form "section\\section\\ ... \\name".
			*  @return returns true on success, false otherwise and error message in value.
			*/
		bool SGetValue(std::string& value, std::string location);

		const std::map<std::string, std::string> GetAllValues(std::string location);
		std::vector<std::string> GetSectionList(std::string location);
		bool SectionExist(std::string location);
		void Test();

		float3 GetFloat3(float3 def, std::string location);
		void LoadBuffer(char* buf, int size);
		AIClasses* ai;


		template<typename T> void ParseArray(std::string value, T* array, int length) {
			std::stringstream stream;
			stream << value;

			for (int i = 0; i < length; i++) {
				stream >> array[i];
			}
		}

		template<typename T> void GetMsg(T& value, const std::string& key) {
			std::stringstream stream;
			stream << SGetValueMSG(key);
			stream >> value;
		}

		template<typename T> void GetDef(T& value, const std::string& defvalue, const std::string& key) {
			std::stringstream stream;
			stream << SGetValueDef(defvalue, key);
			stream >> value;
		}


	private:
		struct SSection {
			std::map<std::string, SSection*> sections;
			std::map<std::string, std::string> values;
		};

		std::map<std::string, SSection*> sections;
		std::string filename;

		void Parse(char* buf, int size);
		void DeleteSection(std::map<std::string, SSection*>* section);
		std::vector<std::string> GetLocationVector(std::string location);
		char* ParseSection(char* buf, int size, SSection* section);
};

#endif
