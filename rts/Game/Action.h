#ifndef ACTION_H
#define ACTION_H

#include <string>

class Action
{
public:
	Action() {};
	Action(const std::string& line);
	std::string command;   // first word, lowercase
	std::string extra;     // everything but the first word
	std::string rawline;   // includes the command, case preserved
	std::string boundWith; // the string that defined the binding keyset
};

#endif
