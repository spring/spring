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

#include <iostream>
#include <string>
#include <boost/program_options.hpp>

#include "Win32Cmd.h"

#include "Util.h"

namespace po = boost::program_options;

/**
 * Just stores the arguments
 */
Win32Cmd::Win32Cmd(int c, char **v)
	:desc("Allowed options")
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
#if defined(USE_OLD_OPTIONS)
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
#else
	for (std::vector<struct option>::iterator it = options.begin(); it != options.end(); it++) {
		std::string optionstr = it->longopt;
		if (it->shortopt)
			optionstr += ","+std::string(&it->shortopt);
		if (it->parmtype == OPTPARM_NONE) {
			desc.add_options()
				(optionstr.c_str(), it->desc.c_str());
		}
		else if (it->parmtype == OPTPARM_INT) {
			desc.add_options()
				(optionstr.c_str(), po::value<int>(),
				it->desc.c_str());
		}
		else if (it->parmtype == OPTPARM_STRING) {
			desc.add_options()
				(optionstr.c_str(), po::value<std::string>(),
				it->desc.c_str());
		}
	}

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	for (std::vector<struct option>::iterator it = options.begin(); it != options.end(); it++) {
		if (vm.count(it->longopt)) {
			it->given = true;
			if (it->parmtype != OPTPARM_NONE) {
				if (it->parmtype == OPTPARM_INT) {
					it->ret.intret = vm[it->longopt].as<int>();
				} else {
					// FIXME memleak
					it->ret.stringret = strdup(vm[it->longopt].as<std::string>().c_str());
				}
			}
		}
	}
#endif
}

/**
 * Print program usage message
 */
void Win32Cmd::usage(std::string program, std::string version)
{
	BaseCmd::usage(program, version);
#if defined(USE_OLD_OPTIONS)
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
#else
	std::cout << desc << std::endl;
#endif
}

