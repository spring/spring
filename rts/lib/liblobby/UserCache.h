/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef USERCACHE_H
#define USERCACHE_H

#include <string>
#include <map>

struct UserInfo
{
	std::string name;
	std::string country;
	int cpu;
	bool ingame;
	bool away;
	int rank;
	bool moderator;
	bool bot;
};

class UserCache
{
public:
	void AddUser(const UserInfo& addition);
	void Update(const UserInfo& update);
	void RemoveUser(const UserInfo& removal);

	UserInfo Get(const std::string& name);
	bool UserExists(const std::string& name);

private:
	typedef std::map<std::string, UserInfo> CacheMap;
	CacheMap users;
};

#endif
