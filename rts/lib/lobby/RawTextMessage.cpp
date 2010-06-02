/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "RawTextMessage.h"

#include <string>
#include <sstream>


RawTextMessage::RawTextMessage(const std::string& _message)
		: message(_message)
		, pos(0)
{}
	
std::string RawTextMessage::GetWord() {

	size_t oldpos = pos;
	pos = message.find_first_of(std::string("\t \n"), oldpos);
	if (pos != std::string::npos) {
		return message.substr(oldpos, pos++ - oldpos);
	} else if (oldpos != std::string::npos) {
		return message.substr(oldpos);
	} else {
		return "";
	}
}
	
std::string RawTextMessage::GetSentence() {

	size_t oldpos = pos;
	pos = message.find_first_of(std::string("\t\n"), oldpos);
	if (pos != std::string::npos) {
		return message.substr(oldpos, pos++ - oldpos);
	} else if (oldpos != std::string::npos) {
		return message.substr(oldpos);
	} else {
		return "";
	}
}
	
int RawTextMessage::GetInt() {

	std::istringstream buf(GetWord());
	int temp;
	buf >> temp;
	return temp;
}

long unsigned RawTextMessage::GetTime() {

	std::istringstream buf(GetWord());
	long unsigned temp;
	buf >> temp;
	return temp;
}
