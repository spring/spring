/*
 * PosixCmd.cpp
 * Posix commandline parser class definition
 * Copyright (C) 2005 Christopher Han
 */
#include "PosixCmd.h"

PosixCmd::PosixCmd(int c, char **v)
{
	argc = c;
	argv = v;
}

void PosixCmd::parse()
{
	addoption('h',"help",OPTPARM_NONE,"","This help message");
	for (int i = 1; i < argc; i++) {
		std::string arg = argv[i];
		if (arg.at(0) == '-') {
			char let = arg.at(1);
			if (let == '-') {
				std::string::size_type ind = arg.find('=',2);
				std::string mainpart = arg.substr(2,ind);
				for (std::vector<struct option>::iterator it = options.begin(); it != options.end(); it++) {
					if (mainpart == it->longopt) {
						it->given = true;
						if (it->parmtype != OPTPARM_NONE) {
							std::string param = "";
							if (arg.size() > mainpart.size()+3) {
								param += arg.substr(ind+1);
							}
							if (it->parmtype == OPTPARM_INT)
								it->intresult = atoi(param.c_str());
							else
								it->stringresult = param;
						}
						break;
					}
				}
			} else {
				for (std::vector<struct option>::iterator it = options.begin(); it != options.end(); it++) {
					if (let == it->shortopt) {
						it->given = true;
						if (it->parmtype != OPTPARM_NONE) {
							std::string next;
							if (arg.size() > 2) {
								next = arg.substr(2);
							} else {
								next = argv[++i];
							}
							if (it->parmtype == OPTPARM_INT)
								it->intresult = atoi(next.c_str());
							else
								it->stringresult = next;
						}
						break;
					}
				}
			}
		}
	}
}

void PosixCmd::usage()
{
	BaseCmd::usage();
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
