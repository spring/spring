/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "TestConnection.h"

#include "lobby/Connection.h"
#include "lobby/RawTextMessage.h"

#include <stdlib.h>
#include <iostream>
#include <string>

void TestConnection::DoneConnecting(bool success, const std::string& err) {

	if (success) {
		std::cout << "Connection established\n";
	} else {
		std::cout << "Could not connect to server: " << err;
		exit(1);
	}
}

void TestConnection::DataReceived(const std::string& command, const std::string& msg) {

	std::cout << command << ": ";
	if (!msg.empty()) {
		RawTextMessage buf(msg);
		std::string par = buf.GetWord();
		while (!par.empty()) {
			std::cout << std::endl << "--> " << par;
			par = buf.GetWord();
		}
	}
	std::cout << std::endl;
}
