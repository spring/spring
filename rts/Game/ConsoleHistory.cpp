/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "ConsoleHistory.h"

CConsoleHistory gameConsoleHistory;

void CConsoleHistory::Init()
{
	lines.clear();
	lines.emplace_back(""); // queue is never empty

	ResetPosition();
}


void CConsoleHistory::ResetPosition()
{
	pos = lines.size();
}


bool CConsoleHistory::AddLine(const std::string& msg)
{
	std::string message;

	if ((msg.find_first_of("aAsS") == 0) && (msg[1] == ':')) {
		message = msg.substr(2);
	} else {
		message = msg;
	}

	return AddLineRaw(message);
}


bool CConsoleHistory::AddLineRaw(const std::string& msg)
{
	// do not save blank lines
	if (msg.empty())
		return false;

	// do not save duplicates
	if (lines.back() == msg)
		return false;

	if (lines.size() >= MAX_LINES) {
		if (pos != 0) {
			lines.pop_front();
		} else {
			lines.pop_front();
			pos = 0;
		}
	}

	lines.push_back(msg);
	return true;
}


std::string CConsoleHistory::NextLine(const std::string& current)
{
	std::string prefix;
	std::string message;

	if ((current.find_first_of("aAsS") == 0) && (current[1] == ':')) {
		prefix  = current.substr(0, 2);
		message = current.substr(2);
	} else {
		message = current;
	}

	if (pos == lines.size()) {
		AddLineRaw(message);
		pos = lines.size();
		return prefix;
	}

	if (lines[pos] != message) {
		if (pos != lines.size() - 1) {
			AddLineRaw(message);
		} else {
			if (AddLineRaw(message)) {
				pos = lines.size();
				return prefix;
			}
		}
	}

	if ((++pos) == lines.size())
		return prefix;

	return prefix + lines[pos];
}


std::string CConsoleHistory::PrevLine(const std::string& current)
{
	std::string prefix;
	std::string message;

	if ((current.find_first_of("aAsS") == 0) && (current[1] == ':')) {
		prefix = current.substr(0, 2);
		message = current.substr(2);
	}
	else {
		message = current;
	}

	if (pos == 0) {
		if (lines[pos] != message) {
			AddLineRaw(message);
			pos = 0;
		}
		return prefix;
	}

	if ((pos == lines.size()) || (lines[pos] != message)) {
		AddLineRaw(message);

		// AddLineRaw() will adjust begin() iterators when it rolls
		if (pos == 0)
			return prefix;
	}

	--pos;

	return prefix + lines[pos];
}