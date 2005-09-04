/*
 * BaseCmd.cpp
 * Base structure for commandline parser class implementation
 * Copyright (C) 2005 Christopher Han <xiphux@gmail.com>
 */
#include "BaseCmd.h"
#ifdef _WIN32
#include "Win32Cmd.h"
#else
#include "PosixCmd.h"
#endif

/**
 * Constructor
 */
BaseCmd::BaseCmd()
{
	addoption('h',"help",OPTPARM_NONE,"","This help message");
}

/**
 * Destructor
 */
BaseCmd::~BaseCmd()
{
}

/**
 * initialize()
 * Returns a new Win32Cmd or PosixCmd instance,
 * depending on the platform
 * @param c argument count
 * @param v array of argument strings
 */
BaseCmd *BaseCmd::initialize(int c, char **v)
{
#ifdef _WIN32
	return new Win32Cmd(c,v);
#else
	return new PosixCmd(c,v);
#endif
}

/**
 * addoption()
 * Adds an option to the list of recognized options
 * @param shortopt short option letter
 * @param longopt long option string
 * @param parmtype type of parameter the argument takes
 * @param desc description string
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
 * deloption()
 * Remove an option from the list of recognized options
 * @param o short option to find
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
 * deloption()
 * Remove an option from the list of recognized options
 * @param o long option to find
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
 * usage()
 * Print usage message
 */
void BaseCmd::usage(std::string program, std::string version)
{
	std::cout << program;
	if (!version.empty())
		std::cout << " " << version;
	std::cout << std::endl;
	std::cout << "This program is licensed under the GNU General Public License" << std::endl;
	std::cout << "Usage: " << argv[0] << " [OPTIONS]" << std::endl;
}

/**
 * result()
 * Gets the post-parse result of an OPTPARM_NONE option
 * @param o short option to get
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
 * result()
 * Gets the post-parse result of an OPTPARM_NONE option
 * @param o long option to get
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
 * result()
 * Gets the post-parse result of an OPTPARM_INT option
 * @param o short option to get
 * @param ret reference to int where result should be stored
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
 * result()
 * Gets the post-parse result of an OPTPARM_INT option
 * @param o long option to get
 * @param ret reference to int where result should be stored
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
 * result()
 * Gets the post-parse result of an OPTPARM_STRING option
 * @param o short option to get
 * @param ret reference to string where result should be stored
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
 * result()
 * Gets the post-parse result of an OPTPARM_STRING option
 * @param o long option to get
 * @param ret reference to string where result should be stored
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
