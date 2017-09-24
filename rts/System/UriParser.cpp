/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "UriParser.h"
#include "StringUtil.h"

static void SplitString(const std::string& text, const char* sepChar, std::string& s1, std::string& s2, std::string& all)
{
	const size_t q = text.find(sepChar);
	if (q != std::string::npos) {
		s1 = text.substr(0, q);
		s2 = text.substr(q + 1);
		return;
	}
	all = text;
}

bool ParseSpringUri(const std::string& uri, std::string& username, std::string& password, std::string& host, int& port)
{
	// see http://cpp-netlib.org/0.10.1/in_depth/uri.html (2014)
	if (uri.find("spring://") == std::string::npos)
		return false; // wrong scheme

	const std::string full = uri.substr(std::string("spring://").length());
	std::string authority, query, user_info, server, portStr;
	bool error = false;
	SplitString(full,      "/", authority, query, authority);
	SplitString(authority, "@", user_info, server, server);
	SplitString(user_info, ":", username, password, username);
	SplitString(server,    ":", host, portStr, host);
	if (portStr.empty())
		return true;
	port = StringToInt(portStr, &error);
	if (error) {
		port = 0;
		return false;
	}
	//FIXME pass query
	return true;
}

bool ParseRapidUri(const std::string& uri, std::string& tag)
{
	if (uri.find("rapid://") == std::string::npos) {
		return false; // wrong scheme
	}
	tag = uri.substr(std::string("rapid://").length());
	return !tag.empty();
}

