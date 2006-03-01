/**
 * @file Win32Cmd.h
 * @brief Win32 commandline base
 * @author Christopher Han <xiphux@gmail.com>
 *
 * Win32 commandline parser class definition
 * Copyright (C) 2005.  Licensed under the terms of the
 * GNU GPL, v2 or later
 */
#ifndef WIN32CMD_H
#define WIN32CMD_H

#include "Platform/BaseCmd.h"

/**
 * @brief Win32 command class
 *
 * Derived from the abstract BaseCmd
 */
class Win32Cmd: public BaseCmd
{
public:
	/**
	 * @brief Constructor
	 * @param c argument count
	 * @param v array of c string arguments
	 */
	Win32Cmd(int c, char **v);

	/**
	 * @brief parse
	 */
	virtual void parse();

	/**
	 * @brief usage
	 * @param program name of the program
	 * @param version version of this program
	 */
	virtual void usage(std::string program, std::string version);

	/**
	 * @brief Destructor
	 */
	~Win32Cmd();
};

#endif /* WIN32CMD_H */
