/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <list>
#include <string>
#include <utility>

typedef std::pair<std::string, unsigned int> TIpPort;
typedef std::list<TIpPort> TStrIntMap;

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
	* check if host & port is in the list for type of allowed, if port is -1, it is ignored
	*/
	bool isAllowed(RestrictType type, const char* hostname, int port=-1);
	/**
	* add resolved ip to the rules where the hostname is used
	*/
	void addIP(const char* hostname, const char* ip);
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
	/**
	* add a rule
	*/
	void addRawRule(RestrictType type, const std::string& hostname, int port);

	TStrIntMap restrictions[ALL_RULES];

};

extern CLuaSocketRestrictions* luaSocketRestrictions;

