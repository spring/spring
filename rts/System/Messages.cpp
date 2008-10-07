/* Author: Tobi Vollebregt */

#include "StdAfx.h"
#include <algorithm>
#include <assert.h>
#include <locale>
#include <cctype>
#include <string>
#include <vector>
#include <map>
#include "mmgr.h"

#include "GlobalStuff.h"
#include "LogOutput.h"
#include "Messages.h"
#include "Lua/LuaParser.h"
#include "System/Util.h"

using std::string;
using std::vector;
using std::map;


CMessages::CMessages(): loaded(false) {}


/** Return a pointer to the single instance of CMessages. */
CMessages* CMessages::GetInstance()
{
	static CMessages instance;
	return &instance;
}


/** Load the messages from gamedata/messages.tdf into memory. */
void CMessages::Load()
{
	LuaParser luaParser("gamedata/messages.lua",
	                    SPRING_VFS_MOD_BASE, SPRING_VFS_MOD_BASE);
	if (!luaParser.Execute()) {
		// Show parse errors in the infolog.
		logOutput.Print(string("ERROR: messages.lua: ") + luaParser.GetErrorLog());
		return;
	}

	const LuaTable root = luaParser.GetRoot();

	vector<string> labels;
	root.GetKeys(labels);

	for (int i = 0; i < labels.size(); i++) {
		const string label = StringToLower(labels[i]);
		const LuaTable msgTable = root.SubTable(label);
		if (!msgTable.IsValid()) {
			continue;
		}
		vector<string> msgs;
		for (int i = 1; true; i++) {
			const string msg = msgTable.GetString(i, "");
			if (msg.empty()) {
				break;
			}
			msgs.push_back(msg);
		}
		if (!msgs.empty()) {
			tr[label] = msgs;
		}
	}
		
	loaded = true;
}


/** Translate \p msg. If multiple messages are available it picks one at random.
Returns \p msg if no messages are available. */
string CMessages::Translate(const string& msg) const
{
	// all keys are lowercase
	const string lower = StringToLower(msg);
	message_map_t::const_iterator it = tr.find(lower);
	if (it == tr.end()) {
		return msg;
	}
	return Pick(it->second);
}


/** Returns a message from \p vec at random. */
string CMessages::Pick(const message_list_t& vec) const
{
	assert(!vec.empty());
	if (vec.size() == 1) {
		return vec[0];
	}
	return vec[gu->usRandInt() % vec.size()];
}
