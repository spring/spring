/*
 * PosixCmd.h
 * Posix commandline parser class definition
 * Copyright (C) 2005 Christopher Han <xiphux@gmail.com>
 */
#ifndef POSIXCMD_H
#define POSIXCMD_H

#include "BaseCmd.h"

class PosixCmd: public BaseCmd
{
public:
	PosixCmd(int c, char **v);
	virtual void parse();
	virtual void usage(std::string program, std::string version);
	~PosixCmd();
private:
	void parselongopt(std::string arg);
};

#endif /* POSIXCMD_H */
