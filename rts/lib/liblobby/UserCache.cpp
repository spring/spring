/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "UserCache.h"

void UserCache::AddUser(const UserInfo& addition)
{
	if (!addition.name.empty() && !UserExists(addition.name))
		users.insert(std::pair<std::string, UserInfo>(addition.name, addition));
}

void UserCache::Update(const UserInfo& update)
{
}

void UserCache::RemoveUser(const UserInfo& removal)
{
	users.erase(removal.name);
}

UserInfo UserCache::Get(const std::string& name)
{
	CacheMap::const_iterator it = users.find(name);
	if (it != users.end())
		return it->second;
	else
	{
		UserInfo unknown;
		unknown.name = name;
		return unknown;
	}
}

bool UserCache::UserExists(const std::string& name)
{
	CacheMap::const_iterator it = users.find(name);
	if (it != users.end())
		return true;
	else
		return false;
}
