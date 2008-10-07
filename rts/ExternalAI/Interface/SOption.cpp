
#ifndef _SOPTION_CPP
#define	_SOPTION_CPP

#include "SOption.h"

#if	defined(__cplusplus) && !defined(BUILDING_AI)
#include "System/Util.h"
#include "Lua/LuaParser.h"
#include "Map/MapParser.h"

#include <set>

static const char* badKeyChars = " =;\r\n\t";


bool ParseOption(const LuaTable& root, int index, Option& opt, std::set<std::string> optionsSet)
{
	const LuaTable& optTbl = root.SubTable(index);
	if (!optTbl.IsValid()) {
		return false;
	}

	// common options properties
	std::string opt_key = optTbl.GetString("key", "");
	if (opt_key.empty() ||
	    (opt_key.find_first_of(badKeyChars) != std::string::npos)) {
		return false;
	}
	opt_key = StringToLower(opt_key);
	if (optionsSet.find(opt_key) != optionsSet.end()) {
		return false;
	}
	opt.key = mallocCopyString(opt_key.c_str());
	
	std::string opt_name = optTbl.GetString("name", opt_key);
	if (opt_name.empty()) {
		return false;
	}
	opt.name = mallocCopyString(opt_name.c_str());
	
	std::string opt_desc = optTbl.GetString("desc", opt_name);
	opt.desc = mallocCopyString(opt_desc.c_str());
	
	std::string opt_section = optTbl.GetString("section", "");
	opt.section = mallocCopyString(opt_section.c_str());
	
	std::string opt_style = optTbl.GetString("style", "");
	opt.style = mallocCopyString(opt_style.c_str());

	std::string opt_type = optTbl.GetString("type", "");
	opt_type = StringToLower(opt_type);
	opt.type = mallocCopyString(opt_type.c_str());

	// option type specific properties
	if (opt_type == "bool") {
		opt.typeCode = opt_bool;
		opt.boolDef = optTbl.GetBool("def", false);
	}
	else if (opt_type == "number") {
		opt.typeCode = opt_number;
		opt.numberDef  = optTbl.GetFloat("def",  0.0f);
		opt.numberMin  = optTbl.GetFloat("min",  -1.0e30f);
		opt.numberMax  = optTbl.GetFloat("max",  +1.0e30f);
		opt.numberStep = optTbl.GetFloat("step", 0.0f);
	}
	else if (opt_type == "string") {
		opt.typeCode = opt_string;
		opt.stringDef    = mallocCopyString(optTbl.GetString("def", "").c_str());
		opt.stringMaxLen = optTbl.GetInt("maxlen", 0);
	}
	else if (opt_type == "list") {
		opt.typeCode = opt_list;

		const LuaTable& listTbl = optTbl.SubTable("items");
		if (!listTbl.IsValid()) {
			return false;
		}

		vector<OptionListItem> opt_list;
		for (int i = 1; listTbl.KeyExists(i); i++) {
			OptionListItem item;

			// string format
			std::string item_key = listTbl.GetString(i, "");
			if (!item_key.empty() &&
			    (item_key.find_first_of(badKeyChars) == string::npos)) {
				item.key = mallocCopyString(item_key.c_str());
				item.name = item.key;
				item.desc = item.name;
				opt_list.push_back(item);
				continue;
			}

			// table format  (name & desc)
			const LuaTable& itemTbl = listTbl.SubTable(i);
			if (!itemTbl.IsValid()) {
				break;
			}
			item_key = itemTbl.GetString("key", "");
			if (item_key.empty() ||
			    (item_key.find_first_of(badKeyChars) != string::npos)) {
				return false;
			}
			item_key = StringToLower(item_key);
			//item.key = item_key.c_str();
			item.key = mallocCopyString(item_key.c_str());
			std::string item_name = itemTbl.GetString("name", item_key);
			if (item_name.empty()) {
				return false;
			}
			//item.name = item_name.c_str();
			item.name = mallocCopyString(item_name.c_str());
			std::string item_desc = itemTbl.GetString("desc", item_name);
			//item.desc = item_desc.c_str();
			item.desc = mallocCopyString(item_desc.c_str());
			opt_list.push_back(item);
		}

		if (opt_list.size() <= 0) {
			return false; // no empty lists
		}
		
		opt.numListItems = opt_list.size();
		opt.list = (OptionListItem*) calloc(sizeof(OptionListItem), opt.numListItems);
        for (int i=0; i < opt.numListItems; ++i) {
            opt.list[i] = opt_list.at(i);
        }

		//opt.listDef = optTbl.GetString("def", opt.list[0].name).c_str();
		opt.listDef = mallocCopyString(optTbl.GetString("def", opt.list[0].name).c_str());
	}
	else {
		return false; // unknown type
	}

	optionsSet.insert(opt.key);

	return true;
}


/*
std::vector<Option> ParseOptions(const std::string& fileName,
                         const std::string& fileModes,
                         const std::string& accessModes,
                         const std::string& mapName)
*/
int ParseOptions(
		const char* fileName,
		const char* fileModes,
		const char* accessModes,
		const char* mapName,
		Option options[], unsigned int max)
{
	//std::vector<Option> options;

	LuaParser luaParser(fileName, fileModes, accessModes);

	const string configName = MapParser::GetMapConfigName(mapName);

	if (!std::string(mapName).empty() && !configName.empty()) {
		luaParser.GetTable("Map");
		luaParser.AddString("fileName", mapName);
		luaParser.AddString("fullName", std::string("maps/") + mapName);
		luaParser.AddString("configFile", configName);
		luaParser.EndTable();
	}
		
	if (!luaParser.Execute()) {
		printf("ParseOptions(%s) ERROR: %s\n",
		       fileName, luaParser.GetErrorLog().c_str());
		return 0;
	}

	const LuaTable root = luaParser.GetRoot();
	if (!root.IsValid()) {
		return 0;
	}

	std::set<std::string> optionsSet;
	int i = 0;
	for (int index = 1; root.KeyExists(index) && i < (int)max; index++) {
		Option opt;
		if (ParseOption(root, index, opt, optionsSet)) {
			options[i++] = opt;
		}
	}

	return i;
}
#endif	/* defined(__cplusplus) && !defined(BUILDING_AI) */

#endif	/* _SOPTION_CPP */
