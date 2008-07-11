/**
 * @file Win32Cmd.cpp
 * @brief Win32 commandline implementation
 * @author Christopher Han <xiphux@gmail.com>
 *
 * Win32 commandline parser class definition
 * Copyright (C) 2005.  Licensed under the terms of the
 * GNU GPL, v2 or later
 */
#include "StdAfx.h"
#include "Win32Cmd.h"

#include <iostream>

/**
 * Just stores the arguments
 */
Win32Cmd::Win32Cmd(int c, char **v)
{
	argc = c;
	argv = v;
}

/**
 * does nothing
 */
Win32Cmd::~Win32Cmd()
{
}

/**
 * Iterates through and processes arguments
 */
void Win32Cmd::parse()
{
	for (int i = 1; i < argc; i++) {
		std::string arg = argv[i];
		if (!arg.empty() && arg.at(0) == '/') {
			if (arg.size() >=2) {
				bool valid = false;
				std::string mainpart = arg.substr(1);
				for (std::vector<struct option>::iterator it = options.begin(); it != options.end(); it++) {
					char so[2];
					SNPRINTF(so, 2, "%c", it->shortopt);
					if (mainpart == it->longopt || mainpart == std::string(so)) {
						it->given = true;
						if (it->parmtype != OPTPARM_NONE) {
							std::string next;
							if (++i >= argc)
								missingparm(arg);
							next = argv[i];
							if (it->parmtype == OPTPARM_INT) {
								if (!is_int(next))
									missingparm(arg);
								it->ret.intret = atoi(next.c_str());
							} else
								it->ret.stringret = argv[i];
						}
						valid = true;
						break;
					}
				}
				if (!valid)
					throw invalidoption(arg);
			} else
				throw invalidoption(arg);
		}
	}
}

/**
 * Print program usage message
 */
void Win32Cmd::usage(std::string program, std::string version)
{
	BaseCmd::usage(program, version);
	for (std::vector<struct option>::iterator it = options.begin(); it != options.end(); it++) {
		std::cout << "\t";
		if (it->shortopt) {
			std::cout << "/" << it->shortopt;
			if (it->parmtype != OPTPARM_NONE)
				std::cout << " [" << it->parmname << "]";
			std::cout << ", ";
		} else {
			std::cout << "  ";
			if (it->parmtype != OPTPARM_NONE)
				std::cout << "\t";
		}
		if (!it->longopt.empty()) {
			std::cout << "/" << it->longopt;
			if (it->parmtype != OPTPARM_NONE)
				std::cout << " [" << it->parmname << "]";
		} else {
			std::cout << "\t";
			if (it->parmtype != OPTPARM_NONE)
				std::cout << "\t";
		}
		std::cout << "\t";
		if (it->longopt.size()<=10)
			std::cout << "\t";
		std::cout << it->desc;
		std::cout << std::endl;
	}
}

