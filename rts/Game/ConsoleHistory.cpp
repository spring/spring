// ConsoleHistory.cpp: implementation of the CConsoleHistory class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "Rendering/GL/myGL.h"

#include "mmgr.h"

#include "ConsoleHistory.h"


unsigned int CConsoleHistory::MaxLines = 256;


CConsoleHistory::CConsoleHistory()
{
	lines.push_back(""); // queue is never empty
	ResetPosition();
	return;
}


CConsoleHistory::~CConsoleHistory()
{
	return;
}


void CConsoleHistory::ResetPosition()
{
	GML_STDMUTEX_LOCK(hist); // ResetPosition

	pos = lines.end();
	return;
}


bool CConsoleHistory::AddLine(const std::string& msg)
{
	GML_STDMUTEX_LOCK(hist); // AddLine

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
	if (msg.empty()) {
		return false; // do not save blank lines
	}
	if (lines.back() == msg) {
		return false; // do not save duplicates
	}
	  
	if (lines.size() >= MaxLines) {
		if (pos != lines.begin()) {
			lines.pop_front();
		} else {
			lines.pop_front();
			pos = lines.begin();
		}
	}

	lines.push_back(msg);

	return true;
}


std::string CConsoleHistory::NextLine(const std::string& current)
{
	GML_STDMUTEX_LOCK(hist); // NextLine

	std::string prefix, message;
	if ((current.find_first_of("aAsS") == 0) && (current[1] == ':')) {
		prefix  = current.substr(0, 2);
		message = current.substr(2);
	} else {
		message = current;
	}
	
	if (pos == lines.end()) {
		AddLineRaw(message);
		pos = lines.end();
		return prefix;
	}

	if (*pos != message) {
		if (pos != --lines.end()) {
			AddLineRaw(message);
		} else {
			if (AddLineRaw(message)) {
				pos = lines.end();
				return prefix;
			}
		}
	}

	pos++;

	if (pos == lines.end()) {
		return prefix;
	}

	return prefix + *pos;
}


std::string CConsoleHistory::PrevLine(const std::string& current)
{
	GML_STDMUTEX_LOCK(hist); // PrevLine

	std::string prefix, message;
	if ((current.find_first_of("aAsS") == 0) && (current[1] == ':')) {
		prefix  = current.substr(0, 2);
		message = current.substr(2);
	} else {
		message = current;
	}
	
	if (pos == lines.begin()) {
		if (*pos != message) {
			AddLineRaw(message);
			pos = lines.begin();
		}
		return prefix;
	}

	if ((pos == lines.end()) || (*pos != message)) {
		AddLineRaw(message);
		if (pos == lines.begin()) {
			return prefix; // AddLineRaw() will adjust begin() iterators when it rolls
		}
	}

	pos--;

	return prefix + *pos;
}
