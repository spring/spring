/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <string>

// url (syntax: spring://username:password@host:port)
bool ParseSpringUri(const std::string& uri, std::string& username, std::string& password, std::string& host, int& port);

// url (syntax: rapid://ba:stable)
bool ParseRapidUri(const std::string& uri, std::string& tag);

