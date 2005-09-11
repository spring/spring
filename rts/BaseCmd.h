/*
 * BaseCmd.h
 * Base structure for commandline parser class definition
 * Copyright (C) 2005 Christopher Han <xiphux@gmail.com>
 */
#ifndef BASECMD_H
#define BASECMD_H

#include <vector>
#include <string>
#include <iostream>

#define OPTPARM_NONE 0
#define OPTPARM_INT 1
#define OPTPARM_STRING 2

class BaseCmd
{
public:
	BaseCmd();
	virtual ~BaseCmd();
	virtual void usage(std::string program, std::string version);
	void addoption(const char shortopt, std::string longopt, unsigned int parmtype, std::string parmname, std::string desc);
	void deloption(const char o);
	void deloption(std::string o);
	virtual void parse() = 0;
	static BaseCmd *initialize(int c, char **v);
	bool result(const char o);
	bool result(std::string o);
	bool result(const char o, int &ret);
	bool result(std::string o, int &ret);
	bool result(const char o, std::string &ret);
	bool result(std::string o, std::string &ret);
protected:
	struct option {
		char shortopt;
		std::string longopt;
		unsigned int parmtype;
		std::string parmname;
		std::string desc;
		bool given;
		union {
			int intret;
			char *stringret;
		} ret;
	};
	std::vector<struct option> options;
	int argc;
	char **argv;
	int invalidoption(std::string opt);
};

#endif
