/*
 Copyright 2008  Nicolas Wu
 
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

/*
#include "AILibrary.h"
#include <string>
#include <iostream>

#include "Util.h"
#include "Platform/FileSystem.h"
#include "Platform/errorhandler.h"
#include "Platform/SharedLib.h"
#include "LogOutput.h"


CAILibrary::CAILibrary(const char* libName, int team) : libName(libName), team(team) {
    init();
}
    
CAILibrary::~CAILibrary() {
    delete lib;
}

void CAILibrary::init() {
    if (!filesystem.GetFilesize(libName)) {
         char msg[512];
         SNPRINTF(msg, 511, "Error loading AI Library \"%s\" : library not found", libName);
         handleerror(NULL, msg, "Error", MBF_OK | MBF_EXCL);
         return;
     }
    logOutput << "Loading AI Library" << libName << "\n";
    
    lib = SharedLib::Instantiate(libName);
    // TODO: version checking
    
    _init = (AI_INIT) lib->FindAddress("init");
    if (_init == 0) {
        char msg[512];
        SNPRINTF(
                msg,
                511,
                "Error loading AI Library \"%s\" : no \"init\" function exported",
                libName);
        handleerror(NULL, msg, "Error", MBF_OK | MBF_EXCL);
        return;
    }
    _init(team);
    
    _handleEvent = (AI_HANDLEEVENT) lib->FindAddress("handleEvent");
    if (_handleEvent == 0) {
        char msg[512];
        SNPRINTF(
                msg,
                511,
                "Error loading AI Library \"%s\" : no \"handleEvent\" function exported",
                libName);
        handleerror(NULL, msg, "Error", MBF_OK | MBF_EXCL);
        return;
    }
}

int CAILibrary::handleEvent(int topic, void* data) {
	return _handleEvent(team, topic, data);
}
*/

