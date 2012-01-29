/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <map>
#include <string>

typedef std::map<std::string, int> TStrIntMap;

class CLuaSocketRestrictions {
public:
	enum RestrictType{
		TCP_CONNECT=0,
		TCP_LISTEN,
		UDP_CONNECT,
		UDP_LISTEN,
		ALL_RULES
	};


	CLuaSocketRestrictions();
	/**
	* check if host & port is in the list for type of allowed
	*/
	bool isAllowed(RestrictType type, const char* hostname, int port);

	/**
	* check if host is allowed
	*/
	bool isAllowedHost(RestrictType type, const char* hostname);
private:
	/**
	* parses and adds a rules string
	* for example
	*     springrts.com:80 springfiles.com:80 123.123.123.123:80
	*/
	void addRules(RestrictType type, const std::string& configstr);
	/**
	* adds a single rule, rule has to be in the format
	*     hostname:port
	* or
	*     hostname:port
	*/
	void addRule(RestrictType type, const std::string& rule);

	TStrIntMap restrictions[ALL_RULES];

};

extern CLuaSocketRestrictions* luaSocketRestrictions;

