/*
 * PosixCmd.cpp
 * Posix commandline parser class definition
 * Copyright (C) 2005 Christopher Han <xiphux@gmail.com>
 */
#include "PosixCmd.h"

/**
 * Constructor
 * @param c argument count
 * @param v array of argument strings
 */
PosixCmd::PosixCmd(int c, char **v): BaseCmd()
{
	argc = c;
	argv = v;
}

/**
 * Destructor
 */
PosixCmd::~PosixCmd()
{
}

/**
 * parselongopt()
 * Parses a long option (--help)
 * @param arg argument as a C++ string
 */
void PosixCmd::parselongopt(std::string arg)
{
	std::string::size_type ind = arg.find('=',2);
	std::string mainpart = arg.substr(2,ind);
	for (std::vector<struct option>::iterator it = options.begin(); it != options.end(); it++) {
		if (mainpart == it->longopt) {
			it->given = true;
			if (it->parmtype == OPTPARM_NONE) {
				if (arg.size() > mainpart.size()+2)
					throw invalidoption(arg);
			} else {
				std::string param = "";
				if (arg.size() > mainpart.size()+3) {
					param += arg.substr(ind+1);
				}
				if (it->parmtype == OPTPARM_INT)
					it->ret.intret = atoi(param.c_str());
				else
					strcpy(it->ret.stringret,param.c_str());
			}
			return;
		}
	}
	throw invalidoption(arg);
}

/**
 * parse()
 * Iterates through and processes arguments
 */
void PosixCmd::parse()
{
	for (int i = 1; i < argc; i++) {
		std::string arg = argv[i];
		if (!arg.empty() && arg.at(0) == '-') {
			if (arg.size() > 2 && arg.at(1) == '-')
				parselongopt(arg);
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
								next = argv[++i];
							}
							if (it->parmtype == OPTPARM_INT)
								it->ret.intret = atoi(next.c_str());
							else
								strcpy(it->ret.stringret,next.c_str());
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
 * usage()
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
		std::cout << "\t\t" << it->desc;
		std::cout << std::endl;
	}
}

/**
 * invalidoption()
 * Prints usage message and terminates when given an invalid option
 * @param opt invalid option
 */
int PosixCmd::invalidoption(std::string opt)
{
	std::cerr << "PosixCmd error: Unrecognized option " << opt << std::endl;
	usage("","");
	exit(1);
	return 1;
}
