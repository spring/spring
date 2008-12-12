/**
 * @file PosixCmd.cpp
 * @brief Posix commandline implementation
 * @author Christopher Han <xiphux@gmail.com>
 *
 * Posix commandline parser class definition
 * Copyright (C) 2005.  Licensed under the terms of the
 * GNU GPL, v2 or later.
 */
#include "PosixCmd.h"

#include <stdlib.h>
#include <iostream>

/**
 * Just stores the arguments and calls the parent class's
 * constructor
 */
PosixCmd::PosixCmd(int c, char **v): BaseCmd()
{
	argc = c;
	argv = v;
}

/**
 * does nothing
 */
PosixCmd::~PosixCmd()
{
}

/**
 * Parses a long string option
 */
void PosixCmd::parselongopt(char* _arg)
{
	std::string arg(_arg);
	std::string::size_type ind = arg.find('=',2);
	std::string mainpart = arg.substr(2,ind-2);
	for (std::vector<struct option>::iterator it = options.begin(); it != options.end(); it++) {
		if (mainpart == it->longopt) {
			it->given = true;
			if (it->parmtype == OPTPARM_NONE) {
				if (ind != std::string::npos)
					throw missingparm(arg);
				if (arg.size() > mainpart.size()+2)
					throw invalidoption(arg);
			} else {
				std::string param = "";
				if (ind == std::string::npos)
					throw missingparm(arg);
				if (arg.size() > mainpart.size()+3) {
					param += arg.substr(ind+1);
				}
				if (it->parmtype == OPTPARM_INT) {
					if (!is_int(param))
						throw missingparm(arg);
					it->ret.intret = atoi(param.c_str());
				} else
					it->ret.stringret = _arg+ind+1;
			}
			return;
		}
	}
	throw invalidoption(arg);
}

/**
 * Iterates through and processes arguments
 */
void PosixCmd::parse()
{
	for (int i = 1; i < argc; i++) {
		std::string arg = argv[i];
		if (!arg.empty() && arg.at(0) == '-') {
			if (arg.size() > 2 && arg.at(1) == '-')
				parselongopt(argv[i]);
			else if (arg.size() >=2) {
				bool valid = false;
				for (std::vector<struct option>::iterator it = options.begin(); it != options.end(); it++) {
					if (arg.at(1) == it->shortopt) {
						if (it->parmtype == OPTPARM_NONE) {
							if (arg.size() <= 2)
								it->given = true;
							else
								throw invalidoption(arg);
						} else {
							it->given = true;
							std::string next;
							if (arg.size() > 2) {
								next = arg.substr(2);
							} else {
								if (++i >= argc)
									throw missingparm(arg);
								next = argv[i];
							}
							if (it->parmtype == OPTPARM_INT) {
								if (!is_int(next))
									throw missingparm(arg);
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
void PosixCmd::usage(std::string program, std::string version)
{
	BaseCmd::usage(program, version);
	for (std::vector<struct option>::iterator it = options.begin(); it != options.end(); it++) {
		std::cout << "\t";
		if (it->shortopt) {
			std::cout << "-" << it->shortopt;
			if (it->parmtype != OPTPARM_NONE)
				std::cout << " [" << it->parmname << "]";
			std::cout << ", ";
		} else {
			std::cout << "  ";
			if (it->parmtype != OPTPARM_NONE)
				std::cout << "\t";
		}
		if (!it->longopt.empty()) {
			std::cout << "--" << it->longopt;
			if (it->parmtype != OPTPARM_NONE)
				std::cout << "=[" << it->parmname << "]";
		} else {
			std::cout << "\t";
			if (it->parmtype != OPTPARM_NONE)
				std::cout << "\t";
		}
		std::cout << "\t";
		if (it->longopt.size()<9)
			std::cout << "\t";
		std::cout << it->desc;
		std::cout << std::endl;
	}
}
