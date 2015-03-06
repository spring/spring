/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef ACTION_H
#define ACTION_H

#include <string>
#include "Game/UI/KeySet.h"

class Action
{
public:
	Action() {}
	Action(const std::string& line);

	std::string command;   ///< first word, lowercase
	std::string extra;     ///< everything but the first word
	std::string rawline;   ///< includes the command, case preserved
	std::string boundWith; ///< the string that defined the binding keyset

	CKeyChain keyChain;    ///< the bounded keychain/keyset
};

#endif // ACTION_H
