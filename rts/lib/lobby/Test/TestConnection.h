/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef TEST_CONNECTION_H
#define TEST_CONNECTION_H

#include "lobby/Connection.h"

#include <string>

class TestConnection : public Connection {
public:
	virtual void DoneConnecting(bool success, const std::string& err);
	virtual void DataReceived(const std::string& command, const std::string& msg);
};

#endif // TEST_CONNECTION_H

