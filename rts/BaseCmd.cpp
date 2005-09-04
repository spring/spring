/*
 * BaseCmd.cpp
 * Base structure for commandline parser class implementation
 */
#include "BaseCmd.h"
#include "PosixCmd.h"

BaseCmd *BaseCmd::initialize(int c, char **v)
{
#ifdef _WIN32
	return new Win32Cmd(c,v);
#else
	return new PosixCmd(c,v);
#endif
}

void BaseCmd::addoption(const char shortopt, std::string longopt, const unsigned int parmtype, std::string parmname, std::string desc)
{
	struct option opt;
	opt.shortopt = shortopt;
	opt.longopt = longopt;
	opt.parmtype = parmtype;
	opt.parmname = parmname;
	opt.desc = desc;
	opt.given = false;
	opt.intresult = 0;
	opt.stringresult = "";
	options.push_back(opt);
}

void BaseCmd::deloption(const char o)
{
	for (std::vector<struct option>::iterator it = options.begin(); it != options.end(); it++) {
		if (it->shortopt == o) {
			options.erase(it);
			break;
		}
	}
}

void BaseCmd::deloption(std::string o)
{
	for (std::vector<struct option>::iterator it = options.begin(); it != options.end(); it++) {
		if (it->longopt == o) {
			options.erase(it);
			break;
		}
	}
}

void BaseCmd::usage()
{
	std::cout << "TA:Spring" << std::endl;
	std::cout << "This program is licensed under the GNU General Public License" << std::endl;
	std::cout << "Usage: " << argv[0] << " [OPTIONS]" << std::endl;
}

bool BaseCmd::result(const char o)
{
	for (std::vector<struct option>::iterator it = options.begin(); it != options.end(); it++) {
		if (it->shortopt == o && it->parmtype == OPTPARM_NONE)
			return it->given;
	}
	return false;
}

bool BaseCmd::result(std::string o)
{
	for (std::vector<struct option>::iterator it = options.begin(); it != options.end(); it++) {
		if (it->longopt == o && it->parmtype == OPTPARM_NONE)
			return it->given;
	}
	return false;
}

bool BaseCmd::result(const char o, int &ret)
{
	for (std::vector<struct option>::iterator it = options.begin(); it != options.end(); it++) {
		if (it->shortopt == o && it->parmtype == OPTPARM_INT) {
			if (it->given) {
				ret = it->intresult;
				return true;
			} else
				return false;
		}
	}
	return false;
}

bool BaseCmd::result(std::string o, int &ret)
{
	for (std::vector<struct option>::iterator it = options.begin(); it != options.end(); it++) {
		if (it->longopt == o && it->parmtype == OPTPARM_INT) {
			if (it->given) {
				ret = it->intresult;
				return true;
			} else
				return false;
		}
	}
	return false;
}

bool BaseCmd::result(const char o, std::string &ret)
{
	for (std::vector<struct option>::iterator it = options.begin(); it != options.end(); it++) {
		if (it->shortopt == o && it->parmtype == OPTPARM_STRING) {
			if (it->given) {
				ret = it->stringresult;
				return true;
			} else
				return false;
		}
	}
	return false;
}

bool BaseCmd::result(std::string o, std::string &ret)
{
	for (std::vector<struct option>::iterator it = options.begin(); it != options.end(); it++) {
		if (it->longopt == o && it->parmtype == OPTPARM_STRING) {
			if (it->given) {
				ret = it->stringresult;
				return true;
			} else
				return false;
		}
	}
	return false;
}
