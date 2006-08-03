/**
 * @file PosixCmd.h
 * @brief Posix commandline base
 * @author Christopher Han <xiphux@gmail.com>
 *
 * Posix commandline parser class definition
 * Copyright (C) 2005.  Licensed under the terms of the
 * GNU GPL, v2 or later.
 */
#ifndef POSIXCMD_H
#define POSIXCMD_H

#include "Platform/BaseCmd.h"

/**
 * @brief Posix command class
 *
 * Derived from the abstract BaseCmd
 */
class PosixCmd: public BaseCmd
{
public:
	/**
	 * @brief Constructor
	 * @param c argument count
	 * @param v array of c string arguments
	 */
	PosixCmd(int c, char **v);

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
	~PosixCmd();
private:

	/**
	 * @brief parse long option
	 * @param arg argument to parse
	 */
	void parselongopt(char* arg);
};

#endif /* POSIXCMD_H */
