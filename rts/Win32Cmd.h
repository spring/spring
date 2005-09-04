/*
 * Win32Cmd.h
 * Win32 commandline parser class definition
 * Copyright (C) 2005 Christopher Han
 */
#ifndef WIN32CMD_H
#define WIN32CMD_H

#include "BaseCmd.h"

class Win32Cmd: public BaseCmd
{
public:
	Win32Cmd(int c, char **v);
	virtual void parse();
	virtual void usage(std::string program, std::string version);
};

#endif /* WIN32CMD_H */
