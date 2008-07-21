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

#ifndef AIEXPORT_H
#define AIEXPORT_H

// First we create extern declarations that will work across
// different platforms.
#ifndef DLL_EXPORT
    #ifdef _WIN32
        #define DLL_EXPORT extern "C" __declspec(dllexport)
    #elif __GNUC__ >= 4
        // Support for '-fvisibility=hidden'.
        #define DLL_EXPORT extern "C" __attribute__ ((visibility("default")))
    #else
        #define DLL_EXPORT extern "C"
    #endif
#endif

DLL_EXPORT int version();
DLL_EXPORT void init(int team);
DLL_EXPORT int handleEvent(int team, int eventID, void* event);

#endif /*AIEXPORT_H*/
