/*
	Copyright (c) 2008 Robin Vobruba <hoijui.quaero@gmail.com>
	
	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Log.h"

#include <stdio.h>	// for file IO
#include <stdlib.h>	// calloc(), exit()
#include <string.h>	// strlen(), strcpy()

const char* myLogFileName = NULL;

void initLog(const char* _logFileName) {
	
	// NOTE: causeing a memory leack, as it is never freed.
	// but it is used till the end of the applications runtime anyway
	// -> no problem
	char* logFileName = (char*) calloc(strlen(_logFileName) + 1, sizeof(char));
	strcpy(logFileName, _logFileName);
	myLogFileName = logFileName;
	
	log("\nlog initialized");
}

void log(const char* msg) {
	
	if (myLogFileName != NULL) {
		FILE* file = fopen(myLogFileName, "a");
		fprintf(file, "%s\n", msg);
		fclose(file);
	} else {
		printf(msg);
	}
}

void logFatalError(const char* msg, int error) {
	
	log(msg);
	exit(error);
}
