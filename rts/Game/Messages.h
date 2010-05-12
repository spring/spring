/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef MESSAGES_H
#define MESSAGES_H

#include <map>
#include <string>
#include <vector>

/**
 * Singleton object to read and store team-death-messages.
 * It essentially behaves like a one-to-many translator:
 * You pass a message to it, and according to the content
 * of messages.lua, it picks a random 'translation' and returns that.
 * In the future this could be extended to support proper internationalisation
 * too.
 * Currently, this is only used for team-death-messages, eg: "team1 is no more"
 */
class CMessages
{
private:
	typedef std::vector<std::string> message_list_t;
	typedef std::map<std::string, message_list_t> message_map_t;
	message_map_t tr;
	bool loaded;
private:
	CMessages();
	static CMessages* GetInstance();
	void Load();
	std::string Translate(const std::string& msg) const;
	std::string Pick(const message_list_t& vec) const;
public:
	static inline std::string Tr(const std::string& msg) {
		CMessages* inst = GetInstance();
		if (!inst->loaded) {
			inst->Load();
		}
		return inst->Translate(msg);
	}
};

#endif // MESSAGES_H
