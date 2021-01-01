/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "restrictions.h"

#include <string>

#include "System/SafeUtil.h"
#include "System/Log/ILog.h"
#include "System/Config/ConfigHandler.h"


#define WILDCARD_HOST "*"
#define WILDCARD_PORT -1

#ifndef TEST
#define LOG_SECTION_LUASOCKET "LuaSocket"
LOG_REGISTER_SECTION_GLOBAL(LOG_SECTION_LUASOCKET)
#ifdef LOG_SECTION_CURRENT
	#undef LOG_SECTION_CURRENT
#endif
#define LOG_SECTION_CURRENT LOG_SECTION_LUASOCKET


CONFIG(std::string, TCPAllowConnect).defaultValue(WILDCARD_HOST).readOnly(true);
CONFIG(std::string, TCPAllowListen).defaultValue(WILDCARD_HOST).readOnly(true);
CONFIG(std::string, UDPAllowConnect).defaultValue("").readOnly(true);
CONFIG(std::string, UDPAllowListen).defaultValue(WILDCARD_HOST).readOnly(true);
#endif


static uint8_t luaSocketRestrictionsMem[sizeof(CLuaSocketRestrictions)];

CLuaSocketRestrictions* luaSocketRestrictions = nullptr;


void CLuaSocketRestrictions::InitStatic() { luaSocketRestrictions = new (luaSocketRestrictionsMem) CLuaSocketRestrictions(); }
void CLuaSocketRestrictions::KillStatic() { spring::SafeDestruct(luaSocketRestrictions); }


CLuaSocketRestrictions::CLuaSocketRestrictions()
{
#ifndef TEST
	addRules(TCP_CONNECT, configHandler->GetString("TCPAllowConnect"));
	addRules(TCP_LISTEN,  configHandler->GetString("TCPAllowListen"));
	addRules(UDP_CONNECT, configHandler->GetString("UDPAllowConnect"));
	addRules(UDP_LISTEN,  configHandler->GetString("UDPAllowListen"));
#endif
}

CLuaSocketRestrictions::~CLuaSocketRestrictions()
{
	LOG("[%s] dumping luasocket rules:", __func__);

	for (int i = 0; i < ALL_RULES; i++) {
		for (const TSocketRule& rule: restrictions[i]) {
			LOG("%s %s %s %d", ruleToStr((RestrictType)i), rule.allowed ? "ALLOW" : "DENY ", rule.hostname.c_str(), rule.port);
		}
	}
}


void CLuaSocketRestrictions::addRule(RestrictType type, const std::string& hostname, int port, bool allowed)
{
	if ((port != WILDCARD_PORT) && ((port <= 0) || (port > 65535))) {
		LOG_L(L_ERROR, "Invalid port specified: %d", port);
		return;
	}

	const TSocketRule* rule = getRule(type, hostname.c_str(), port);

	if ((rule != nullptr) && (rule->allowed == allowed)) {
		LOG_L(L_ERROR, "Rule already exists: %s %d", hostname.c_str(), port);
		return;
	}

	// add deny-rules to the front of the list
	if (!allowed) {
		restrictions[type].emplace_front(hostname, port, allowed);
	} else {
		restrictions[type].emplace_back(hostname, port, allowed);
	}
}

void CLuaSocketRestrictions::addRule(RestrictType type, const std::string& rule)
{
	// allow everything
	if (rule == WILDCARD_HOST) {
		restrictions[type].emplace_back(WILDCARD_HOST, WILDCARD_PORT, true);
		return;
	}

	const size_t delimpos = rule.find(":");

	if (delimpos == std::string::npos) {
		LOG_L(L_ERROR, "Invalid %d rule: %s, rule has to be hostname:port", type, rule.c_str());
		return;
	}

	const std::string strport = rule.substr(delimpos + 1, delimpos - rule.length());
	const int port = atoi(strport.c_str());

	addRule(type, rule.substr(0, delimpos), port, true);
}

void CLuaSocketRestrictions::addRules(RestrictType type, const std::string& configstr)
{
	std::string rule;

	for (size_t i = 0; i < configstr.length(); i++) {
		const char ch = configstr[i];

		if ((isspace(ch)) && (!rule.empty())) {
			addRule(type, rule);
			rule = "";
		} else {
			rule += ch;
		}
	}

	if (rule.empty())
		return;

	addRule(type, rule);
}

/*
bool isValidIpAddress(const char* ipAddress)
{
	struct sockaddr_in sa;
	return (inet_pton(AF_INET, ipAddress, &(sa.sin_addr)) != 0);
}
*/

bool CLuaSocketRestrictions::isAllowed(RestrictType type, const char* hostname, int port) {
	const TSocketRule* rule = getRule(type, hostname, port);

	if (rule == nullptr)
		return false;

	return rule->allowed;
}

const TSocketRule* CLuaSocketRestrictions::getRule(RestrictType type, const char* hostname, int port) {
	int start;
	int end;

	if (type != ALL_RULES) {
		start = type;
		end = start + 1;
	} else {
		start = 0;
		end = ALL_RULES;
	}

	for (int i = start; i < end; i++) {
		for (const TSocketRule& rule: restrictions[i]) {
			if ((hostname != rule.hostname) && (rule.hostname != WILDCARD_HOST))
				continue;

			if ((rule.port != WILDCARD_PORT) && (rule.port != port))
				continue;

			return &rule;
		}
	}

	return nullptr;
}

void CLuaSocketRestrictions::addIP(const char* hostname, const char* ip)
{
	for (int i = 0; i < ALL_RULES; i++) {
		for (const TSocketRule& rule: restrictions[i]) {
			if (rule.hostname != hostname)
				continue;
			if (getRule((RestrictType)i, ip, rule.port) != nullptr)
				continue;

			// add rule with ip if one does not already exist
			addRule((RestrictType)i, ip, rule.port, rule.allowed);
			//?? break;
		}
	}
}

const char* CLuaSocketRestrictions::ruleToStr(RestrictType type) {
	switch (type) {
		case TCP_CONNECT: return "TCP_CONNECT";
		case TCP_LISTEN : return "TCP_LISTEN ";
		case UDP_LISTEN : return "UDP_LISTEN ";
		case UDP_CONNECT: return "UDP_CONNECT";
		default: return "INVALID";
	}
}

