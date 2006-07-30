// ConsoleHistory.cpp: implementation of the CConsoleHistory class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "ConsoleHistory.h"

unsigned int CConsoleHistory::MaxLines = 256;


CConsoleHistory::CConsoleHistory()
{
	ResetPosition();
	return;
}


CConsoleHistory::~CConsoleHistory()
{
	return;
}


void CConsoleHistory::ResetPosition()
{
	pos = lines.end();
	return;
}


bool CConsoleHistory::AddLine(const string& msg)
{
	if (msg.empty()) {
		return false; // do not save blank lines
	}
	if (!lines.empty() && (lines.back() == msg)) {
		return false; // do not save duplicates
	}
	  
	if (lines.size() >= MaxLines) {
		if (pos == lines.begin()) {
		pos++;
		}
		lines.pop_front();
	}

	lines.push_back(msg);

	return true;
}


string CConsoleHistory::NextLine(const string& current)
{
	if (pos == lines.end()) {
		AddLine(current);
		pos = lines.end();
		return "";
	}
	if (*pos != current) {
		AddLine(current);
	}
	pos++;
	if (pos == lines.end()) {
		return "";
	}
	return *pos;
}


string CConsoleHistory::PrevLine(const string& current)
{
	if (pos == lines.begin()) {
		AddLine(current);
		return "";
	}
	if ((pos == lines.end()) || (*pos != current)) {
		AddLine(current);
		if (pos == lines.begin()) {
		return ""; // AddLine() will adjust begin() iterators when it shifts
		}
	}
	pos--;
	if (pos == lines.begin()) {
		return "";
	}
	return *pos;
}
