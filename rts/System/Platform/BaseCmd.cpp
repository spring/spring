/**
 * @file BaseCmd.cpp
 * @brief Abstract commandline implementation
 * @author Christopher Han <xiphux@gmail.com>
 *
 * Base structure for commandline parser class implementation
 * Copyright (C) 2005. Licensed under the terms of the
 * GNU GPL, v2 or later.
 */
#include "StdAfx.h"
#include "BaseCmd.h"

#include <iostream>

#ifdef _WIN32
#include "Win/Win32Cmd.h"
#else
#include "Linux/PosixCmd.h"
#endif

/**
 * The base constructor sets up the default help and
 * version options that should be available on all platforms.
 */
BaseCmd::BaseCmd()
{
	addoption('h',"help",OPTPARM_NONE,"","This help message");
	addoption('V',"version",OPTPARM_NONE,"","Display program version");
}

/**
 * The destructor currently does nothing
 */
BaseCmd::~BaseCmd()
{
}

/**
 * This is the static initialization function that
 * will instantiate a platform-specific derived commandline
 * class appropriate for this platform
 */
BaseCmd *BaseCmd::initialize(int c, char **v)
{
#ifdef _WIN32
	return SAFE_NEW Win32Cmd(c,v);
#else
	return SAFE_NEW PosixCmd(c,v);
#endif
}

/**
 * Adds a parameter to the list of recognized options
 */
void BaseCmd::addoption(const char shortopt, std::string longopt, const unsigned int parmtype, std::string parmname, std::string desc)
{
	struct option opt;
	opt.shortopt = shortopt;
	opt.longopt = longopt;
	opt.parmtype = parmtype;
	opt.parmname = parmname;
	opt.desc = desc;
	opt.given = false;
	options.push_back(opt);
}

/**
 * Remove the option that has the given short flag
 */
void BaseCmd::deloption(const char o)
{
	for (std::vector<struct option>::iterator it = options.begin(); it != options.end(); it++) {
		if (it->shortopt == o) {
			options.erase(it);
			break;
		}
	}
}

/**
 * Remove the option that has the given long flag
 */
void BaseCmd::deloption(std::string o)
{
	for (std::vector<struct option>::iterator it = options.begin(); it != options.end(); it++) {
		if (it->longopt == o) {
			options.erase(it);
			break;
		}
	}
}

/**
 * Print out usage information, along with a list of commandline
 * options that have been set up for this commandline parser.
 */
void BaseCmd::usage(std::string program, std::string version)
{
	if (!program.empty()) {
		std::cout << program;
		if (!version.empty())
			std::cout << " " << version;
		std::cout << std::endl;
		std::cout << "This program is licensed under the GNU General Public License" << std::endl;
	}
	std::cout << "Usage: " << argv[0] << " [OPTIONS]" << std::endl;
}

/**
 * Gets the post-parse result of an OPTPARM_NONE option
 */
bool BaseCmd::result(const char o)
{
	for (std::vector<struct option>::iterator it = options.begin(); it != options.end(); it++) {
		if (it->shortopt == o && it->parmtype == OPTPARM_NONE)
			return it->given;
	}
	return false;
}

/**
 * Gets the post-parse result of an OPTPARM_NONE option
 */
bool BaseCmd::result(std::string o)
{
	for (std::vector<struct option>::iterator it = options.begin(); it != options.end(); it++) {
		if (it->longopt == o && it->parmtype == OPTPARM_NONE)
			return it->given;
	}
	return false;
}

/**
 * Gets the post-parse result of an OPTPARM_INT option
 */
bool BaseCmd::result(const char o, int &ret)
{
	for (std::vector<struct option>::iterator it = options.begin(); it != options.end(); it++) {
		if (it->shortopt == o && it->parmtype == OPTPARM_INT) {
			if (it->given)
				ret = it->ret.intret;
			return it->given;
		}
	}
	return false;
}

/**
 * Gets the post-parse result of an OPTPARM_INT option
 */
bool BaseCmd::result(std::string o, int &ret)
{
	for (std::vector<struct option>::iterator it = options.begin(); it != options.end(); it++) {
		if (it->longopt == o && it->parmtype == OPTPARM_INT) {
			if (it->given)
				ret = it->ret.intret;
			return it->given;	
		}
	}
	return false;
}

/**
 * Gets the post-parse result of an OPTPARM_STRING option
 */
bool BaseCmd::result(const char o, std::string &ret)
{
	for (std::vector<struct option>::iterator it = options.begin(); it != options.end(); it++) {
		if (it->shortopt == o && it->parmtype == OPTPARM_STRING) {
			if (it->given)
				ret = it->ret.stringret;
			return it->given;
		}
	}
	return false;
}

/**
 * Gets the post-parse result of an OPTPARM_STRING option
 */
bool BaseCmd::result(std::string o, std::string &ret)
{
	for (std::vector<struct option>::iterator it = options.begin(); it != options.end(); it++) {
		if (it->longopt == o && it->parmtype == OPTPARM_STRING) {
			if (it->given)
				ret = it->ret.stringret;
			return it->given;
		}
	}
	return false;
}

/**
 * When an unrecognized option is given, this is called.
 * Prints out an extra "unrecognized option blah" along
 * with the usage message.
 */
int BaseCmd::invalidoption(std::string opt)
{
	std::cerr << "BaseCmd error: Unrecognized option " << opt << std::endl;
	usage("","");
	exit(1);
	return 1;
}

/**
 * When an option is missing a parameter, this is called.
 * Prints out an extra "unrecognized option blah" along
 * with the usage message.
 */
int BaseCmd::missingparm(std::string opt)
{
	std::cerr << "BaseCmd error: Invalid or missing parameter for option " << opt << std::endl;
	usage("","");
	exit(1);
	return 1;
}

/**
 * Checks whether string s is an integer.
 */
bool BaseCmd::is_int(const std::string& s) const
{
	if (!isdigit(s[0]) && s[0] != '-' && s[0] != '+')
		return false;
	for (int j = 1; j < s.size(); ++j)
		if (!isdigit(s[j]))
			return false;
	return true;
}
