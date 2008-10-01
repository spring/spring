// The structs in this files relate to *Options.lua files
// They are used for Mods and Skirmish AIs eg

#ifndef _SOPTION_H
#define	_SOPTION_H

#ifdef	__cplusplus
extern "C" {
#endif

struct OptionListItem {
	const char* key;
	const char* name;
	const char* desc;
};

enum OptionType {
	opt_error  = 0,
	opt_bool   = 1,
	opt_list   = 2,
	opt_number = 3,
	opt_string = 4
};

struct Option {
#ifdef	__cplusplus
	Option() : typeCode(opt_error) {}
#endif

	const char* key;
	const char* name;
	const char* desc;

	const char* type; // "bool", "number", "string", "list", ... (see enum OptionType)

	OptionType typeCode;

	bool   boolDef;

	float  numberDef;
	float  numberMin;
	float  numberMax;
	float  numberStep; // aligned to numberDef

	const char* stringDef;
	int    stringMaxLen;

	const char* listDef;
	//vector<ListItem> list;
	int numListItems;
	OptionListItem* list;
};

#if	defined(__cplusplus) && !defined(BUILDING_AI)
int ParseOptions(
		const char* fileName,
		const char* fileModes,
		const char* accessModes,
		const char* mapName,
		Option options[], unsigned int max);
#endif	/* defined(__cplusplus) && !defined(BUILDING_AI) */

#ifdef	__cplusplus
}
#endif


//#ifdef	__cplusplus
//#include <vector>
//#include <string>
//std::vector<Option> ParseOptions(const std::string& fileName,
//                         const std::string& fileModes,
//                         const std::string& accessModes,
//                         const std::string& mapName = "");
//#endif


#endif	/* _SOPTION_H */

