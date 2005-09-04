/*
 * Win32Cmd.cpp
 * Win32 commandline parser class definition
 * Copyright (C) 2005 Christopher Han
 */
#include "Win32Cmd.h"

Win32Cmd::Win32Cmd(int c, char **v)
{
	argc = c;
	argv = v;
}

void Win32Cmd::parse()
{
}

void Win32Cmd::usage(std::string program, std::string version)
{
	BaseCmd::usage(program, version);
}
