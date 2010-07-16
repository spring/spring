/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef RAW_TEXT_MESSAGE_H
#define RAW_TEXT_MESSAGE_H

#include <string>

class RawTextMessage {
public:
	RawTextMessage(const std::string& _message);
	std::string GetWord();
	std::string GetSentence();
	int GetInt();
	long unsigned GetTime();

private:
	std::string message;
	size_t pos;
};

#endif // RAW_TEXT_MESSAGE_H

