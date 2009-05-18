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
#include <cstdlib>
#include <cstdio>

namespace po = boost::program_options;

/**
 * The base constructor sets up the default help and
 * version options that should be available on all platforms.
 */
BaseCmd::BaseCmd(int _argc, char* _argv[]) : desc("Allowed options")
{
	argc = _argc;
	argv = _argv;
	addoption('h',"help",OPTPARM_NONE,"","This help message");
	addoption('V',"version",OPTPARM_NONE,"","Display program version");
}

/**
 * The destructor currently does nothing
 */
BaseCmd::~BaseCmd()
{
}

std::string BaseCmd::GetInputFile()
{
	if (vm.count("input-file"))
		return vm["input-file"].as<std::string>();
	else
		return "";
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
 * Iterates through and processes arguments
 */
void BaseCmd::parse()
{
	boost::program_options::options_description all;
	for (std::vector<struct option>::iterator it = options.begin(); it != options.end(); it++) {
		std::string optionstr = it->longopt;
		if (it->shortopt) {
			char str[4];
			sprintf(str,",%c",it->shortopt);
			optionstr += str;
		}
		if (it->parmtype == OPTPARM_NONE) {
			all.add_options()(optionstr.c_str(), it->desc.c_str());
		}
		else if (it->parmtype == OPTPARM_INT) {
			all.add_options()(optionstr.c_str(), po::value<int>(), it->desc.c_str());
		}
		else if (it->parmtype == OPTPARM_STRING) {
			all.add_options()(optionstr.c_str(), po::value<std::string>(), it->desc.c_str());
		}
	}
	desc.add(all);

	all.add_options()("input-file", po::value<std::string>(), "input file");
	po::positional_options_description p;
	p.add("input-file", 1);

	po::store(po::command_line_parser(argc, argv).options(all).positional(p).run(), vm);
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
	std::cout << desc << std::endl;
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
	for (unsigned int j = 1; j < s.size(); ++j)
		if (!isdigit(s[j]))
			return false;
	return true;
}
