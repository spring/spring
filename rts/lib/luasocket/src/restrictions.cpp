/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "restrictions.h"

#include <map>
#include <string>

#include "System/Log/ILog.h"
#include "System/Config/ConfigHandler.h"


CONFIG(std::string, TCPAllowConnect).defaultValue("").readOnly(true);
CONFIG(std::string, TCPAllowListen).defaultValue("").readOnly(true);
CONFIG(std::string, UDPAllowConnect).defaultValue("").readOnly(true);
CONFIG(std::string, UDPAllowListen).defaultValue("").readOnly(true);

CLuaSocketRestrictions* luaSocketRestrictions=0;

CLuaSocketRestrictions::CLuaSocketRestrictions()
{
	addRules(TCP_CONNECT, configHandler->GetString("TCPAllowConnect"));
	addRules(TCP_LISTEN,  configHandler->GetString("TCPAllowListen"));
	addRules(UDP_CONNECT, configHandler->GetString("UDPAllowConnect"));
	addRules(UDP_LISTEN,  configHandler->GetString("UDPAllowListen"));
}

void CLuaSocketRestrictions::addRule(RestrictType type, const std::string& rule)
{
	size_t delimpos = rule.find(":");
	if (delimpos==std::string::npos) {
		LOG_L(L_ERROR, "Invalid %d rule: %s, rule has to be hostname:port", type, rule.c_str());
		return;
	}
	const std::string strport = rule.substr(delimpos+1, delimpos-rule.length());
	int port = atoi(strport.c_str());
	if ((port<=0) || (port>65535)){
		LOG_L(L_ERROR, "Invalid port specified: %s", strport.c_str());
		return;
	}
	LOG_L(L_WARNING, "Adding rule %d %s:%d",type, rule.substr(0, delimpos).c_str(), port);
	restrictions[type][rule.substr(0, delimpos)] = port;
}


void CLuaSocketRestrictions::addRules(RestrictType type, const std::string& configstr)
{
	int i=0;
	char ch;
	std::string rule;

	while((ch=configstr[i++])) {
		if ((isspace(ch)) && (!rule.empty())) {
			addRule(type, rule);
			rule = "";
		} else {
			rule += ch;
		}
	}
	if (!rule.empty()) {
		addRule(type, rule);
	}
}

/*
bool isValidIpAddress(char *ipAddress)
{
    struct sockaddr_in sa;
    int result = inet_pton(AF_INET, ipAddress, &(sa.sin_addr));
    return result != 0;
}
*/

bool CLuaSocketRestrictions::isAllowed(RestrictType type, const char* hostname, int port)
{
	int start, end;
	if (type!=ALL_RULES) {
		start = type;
		end = start+1;
	} else {
		start = 0;
		end = ALL_RULES;
	}
	for (int i=start; i<end; i++) {
		TStrIntMap::iterator it = restrictions[type].find(hostname);
		if (it ==restrictions[i].end()) //check if host / ip exists in array
			continue;
		const int rport = (*it).second;
		if (port==-1) { // port ignored
			return true;
		} else if (rport==port) {
			return true;
		}
	}
	return false;
}

