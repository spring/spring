// <copyright file="SOP.hpp">
//
// Copyright (c) 2012, Daniel Cornel. Published on drivenbynostalgia.com.
// All rights reserved.
//
// </copyright>
// <license>
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//    * Redistributions of source code must retain the above copyright
//      notice, this list of conditions and the following disclaimer.
//    * Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimer in the
//      documentation and/or other materials provided with the distribution.
//    * Neither the name of the copyright holder nor the
//      names of its contributors may be used to endorse or promote products
//      derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 
// </license>

#ifndef _SOP_HPP
#define _SOP_HPP

#include <algorithm>
#include <string>

// Return values
#define SOP_RESULT_NO_CHANGE 0
#define SOP_RESULT_CHANGE 1
#define SOP_RESULT_ERROR -1

// Descriptor for an application bound to an application profile
struct Application {
    unsigned long version;
    unsigned long isPredefined;
    unsigned short appName[2048];
    unsigned short userFriendlyName[2048];
    unsigned short launcher[2048];
    unsigned short fileInFolder[2048];
};

// Application profile descriptor
struct Profile {
    unsigned long version;
    unsigned short profileName[2048];
    unsigned long* gpuSupport;
    unsigned long isPredefined;
    unsigned long numOfApps;
    unsigned long numOfSettings;
};

// Setting descriptor
struct Setting {
    struct BinarySetting {
        unsigned long valueLength;
        unsigned char valueData[4096];
    };

    unsigned long version;
    unsigned short settingName[2048];
    unsigned long settingID;
    unsigned long settingType;
    unsigned long settingLocation;
    unsigned long isCurrentPredefined; 
    unsigned long isPredefinedValid;

    union {
        unsigned long u32PredefinedValue;
        BinarySetting binaryPredefinedValue;
        unsigned short strPredefinedValue[2048];
    };

    union {
        unsigned long u32CurrentValue;
        BinarySetting binaryCurrentValue;
        unsigned short strCurrentValue[2048];
    };
};

// Definitions for required NvAPI functions
typedef int (*CreateApplicationT)(int session, int profile, Application* application);
CreateApplicationT CreateApplication = NULL;

typedef int (*CreateProfileT)(int session, Profile* profileInfo, int* profile);
CreateProfileT CreateProfile = NULL;

typedef int (*CreateSessionT)(int* session);
CreateSessionT CreateSession = NULL;

typedef int (*DeleteProfileT)(int session, int profile);
DeleteProfileT DeleteProfile = NULL;

typedef int (*DestroySessionT)(int session);
DestroySessionT DestroySession = NULL;

typedef int (*EnumApplicationsT)(int session, int profile, unsigned long startIndex, unsigned long* appCount, Application* application);
EnumApplicationsT EnumApplications = NULL;

typedef int (*FindProfileByNameT)(int session, unsigned short profileName[2048], int* profile);
FindProfileByNameT FindProfileByName = NULL;

typedef int (*GetProfileInfoT)(int session, int profile, Profile* profileInfo);
GetProfileInfoT GetProfileInfo = NULL;

typedef int (*LoadSettingsT)(int session);
LoadSettingsT LoadSettings = NULL;

typedef int (*SaveSettingsT)(int session);
SaveSettingsT SaveSettings = NULL;

typedef int (*SetSettingT)(int session, int profile, Setting* setting);
SetSettingT SetSetting = NULL;

typedef int (*InitializeT)();
InitializeT Initialize = NULL;

typedef int* (*QueryInterfaceT)(unsigned int offset);
QueryInterfaceT QueryInterface = NULL;

bool CheckForError(int status) {
    if (status != 0) {

#if _DEBUG
        fprintf(stderr, "NvAPI error in SetOptimusProfile: %i\n", status);
#endif

        return true;
    }
    else {
        return false;
    }
}

bool UnicodeStringCompare(unsigned short firstString[2048], unsigned short secondString[2048]) {
    for (int i = 0; i < 2048; i++) {
        if (firstString[i] != secondString[i]) {
            return false;
        }
    }

    return true;
}

void GetUnicodeString(std::string sourceString, unsigned short (* destinationString)[2048]) {
    for (unsigned int i = 0; i < 2048; i++) {
        if (i < sourceString.length()) {
            (*destinationString)[i] = (unsigned short) sourceString[i];
        }
        else {
            (*destinationString)[i] = 0;
        }
    }
}

bool GetProcs() {
    // Check if this is a 32 bit application
    if (sizeof(void*) != 4) {
#if _DEBUG
        fprintf(stderr, "Only 32 bit applications are supported.");
#endif

        return false;
    }

    HMODULE hMod = LoadLibraryA("nvapi.dll");
    
    // Check if the nvapi.dll is available
    if (hMod == NULL) {
#if _DEBUG
        fprintf(stderr, "The nvapi.dll could not be found.");
#endif

        return false;
    }

    // This function is used to get all other function procs
    QueryInterface = (QueryInterfaceT) GetProcAddress(hMod, "nvapi_QueryInterface");

#define SafeQueryInterface(var, type, value) \
	var = (type) (*QueryInterface)(value); \
	if (var == NULL) return false;
    // Query the procs with an ID
    // the IDs can be retrieved by parsing the nvapi.lib such that no library has to be linked
    SafeQueryInterface(CreateApplication, CreateApplicationT, 0x4347A9DE);
    SafeQueryInterface(CreateProfile,     CreateProfileT,     0xCC176068);
    SafeQueryInterface(CreateSession,     CreateSessionT,     0x0694D52E);
    SafeQueryInterface(DeleteProfile,     DeleteProfileT,     0x17093206);
    SafeQueryInterface(DestroySession,    DestroySessionT,    0xDAD9CFF8);
    SafeQueryInterface(EnumApplications,  EnumApplicationsT,  0x7FA2173A);
    SafeQueryInterface(FindProfileByName, FindProfileByNameT, 0x7E4A9A0B);
    SafeQueryInterface(GetProfileInfo,    GetProfileInfoT,    0x61CD6FD6);
    SafeQueryInterface(LoadSettings,      LoadSettingsT,      0x375DBD6B);
    SafeQueryInterface(SaveSettings,      SaveSettingsT,      0xFCBC7E14);
    SafeQueryInterface(SetSetting,        SetSettingT,        0x577DD202);
    SafeQueryInterface(Initialize,        InitializeT,        0x0150E828);

    return true;
}

bool ContainsApplication(int session, int profile, Profile profileDescriptor, unsigned short applicationName[2048], Application* application) {
    if (profileDescriptor.numOfApps == 0) {
        return false;
    }

    // Iterate over all applications bound to the profile
    unsigned long numAppsRead = profileDescriptor.numOfApps;
    Application* allApplications = new Application[profileDescriptor.numOfApps];
    allApplications[0].version = 147464; // Calculated from the size of the descriptor
    
    if (CheckForError((*EnumApplications)(session, profile, 0, &numAppsRead, allApplications))) {
        delete[] allApplications;

        return false;
    }

    for (unsigned int i = 0; i < numAppsRead; i++) {
        if (UnicodeStringCompare(allApplications[i].appName, applicationName)) {
            application = &allApplications[i];

            return true;
        }
    }

    delete[] allApplications;

    return false;
}

// Call this from your application to check if an application profile with
// the name provided exists.
bool SOP_CheckProfile(std::string profileNameString) {
    bool result = false;
    int session = 0;
    int profile = 0;
    
    // Initialize NvAPI
    if ((!GetProcs()) || (CheckForError((*Initialize)()))) {
        return false;
    }
    
    // Create a unicode string from the profile name
    unsigned short profileName[2048];
    GetUnicodeString(profileNameString, &profileName);

    // Create the session handle to access driver settings
    if (CheckForError((*CreateSession)(&session))) {
        return false;
    }

    // Load all the system settings into the session
    if (CheckForError((*LoadSettings)(session))) {
        return false;
    }

    // Check if the application profile with the specified name exists
    result = ((*FindProfileByName)(session, profileName, &profile) == 0);
    
    (*DestroySession)(session);
    session = 0;

    return result;
}

// Call this from your application to delete the application profile with
// the name provided. Note that only one application profile per name exists
// for all applications bound to it.
// After the profile has been erased, the application will use the default GPU
// the next time it is started, usually the integrated GPU.
int SOP_RemoveProfile(std::string profileNameString) {
    int result = SOP_RESULT_NO_CHANGE;
    int status = 0;
    int session = 0;
    int profile = 0;
    
    // Initialize NvAPI
    if ((!GetProcs()) || (CheckForError((*Initialize)()))) {
        return SOP_RESULT_ERROR;
    }
    
    // Create a unicode string from the profile name
    unsigned short profileName[2048];
    GetUnicodeString(profileNameString, &profileName);

    // Create the session handle to access driver settings
    if (CheckForError((*CreateSession)(&session))) {
        return SOP_RESULT_ERROR;
    }

    // Load all the system settings into the session
    if (CheckForError((*LoadSettings)(session))) {
        return SOP_RESULT_ERROR;
    }

    // Check if the application profile with the specified name already exists
    status = (*FindProfileByName)(session, profileName, &profile);

    if (status == 0) {
        // The application profile with the specified name exists and can be deleted
        if ((CheckForError((*DeleteProfile)(session, profile))) ||
            CheckForError((*SaveSettings)(session))) {
            return SOP_RESULT_ERROR;
        }
        else {
            result = SOP_RESULT_CHANGE;
        }
    }
    else if (status == -163 /* Profile not found */) {
        // The application profile does not exist and does not have to be deleted
        result = SOP_RESULT_NO_CHANGE;
    }
    else {
        return SOP_RESULT_ERROR;
    }
    
    (*DestroySession)(session);
    session = 0;

    return result;
}

// Call this from your application to create a generic application profile with
// the name provided and add the application name to it. The application profile is
// set to start all bound applications with the discrete (NVIDIA) GPU. If the profile
// already exists because it is shared among several applications, the provided
// application name is bound to the existing profile.
// The changes take effect the next time the application is started.
int SOP_SetProfile(std::string profileNameString, std::string applicationNameString) {
    int result = SOP_RESULT_NO_CHANGE;
    int status = 0;
    int session = 0;
    int profile = 0;
    
    // Initialize NvAPI
    if ((!GetProcs()) || (CheckForError((*Initialize)()))) {
        return SOP_RESULT_ERROR;
    }

    // Create a unicode string from the profile name
    unsigned short profileName[2048];
    GetUnicodeString(profileNameString, &profileName);
    
    // Create a unicode string from the application name
    std::transform(applicationNameString.begin(), applicationNameString.end(), applicationNameString.begin(), ::tolower);
    unsigned short applicationName[2048];
    GetUnicodeString(applicationNameString, &applicationName);

    // Create the session handle to access driver settings
    if (CheckForError((*CreateSession)(&session))) {
        return SOP_RESULT_ERROR;
    }

    // Load all the system settings into the session
    if (CheckForError((*LoadSettings)(session))) {
        return SOP_RESULT_ERROR;
    }
    
    // Check if the application profile with the specified name already exists
    status = (*FindProfileByName)(session, profileName, &profile);

    if (status == -163 /* Profile not found */) {
        // The application profile does not yet exist and has to be created
        Profile newProfileDescriptor = { 0 };
        newProfileDescriptor.version = 69652; // Calculated from the size of the descriptor
        newProfileDescriptor.isPredefined = 0;
        memcpy(&newProfileDescriptor.profileName, &profileName, 2048);
        newProfileDescriptor.gpuSupport = new unsigned long[32];
        newProfileDescriptor.gpuSupport[0] = 1;

        // Create the application profile
        if (CheckForError((*CreateProfile)(session, &newProfileDescriptor, &profile))) {
            delete newProfileDescriptor.gpuSupport;

            return SOP_RESULT_ERROR;
        }

        // Create the application settings. This is where the discrete GPU for Optimus is set.
        Setting optimusSetting = { 0 };
        optimusSetting.version = 77856; // Calculated from the size of the descriptor
        optimusSetting.settingID = 0x10F9DC81; // Shim rendering mode ID
        optimusSetting.u32CurrentValue = 0x00000001 | 0x00000010; // Enable | Auto select
        
        if (CheckForError((*SetSetting)(session, profile, &optimusSetting))) {
            delete newProfileDescriptor.gpuSupport;

            return SOP_RESULT_ERROR;
        }

        delete newProfileDescriptor.gpuSupport;
    }
    else if (CheckForError(status)) {
        return SOP_RESULT_ERROR;
    }

    // Retrieve the profile information of the application profile
    Profile profileDescriptor = { 0 };
    profileDescriptor.version = 69652; // Calculated from the size of the descriptor

    if (CheckForError((*GetProfileInfo)(session, profile, &profileDescriptor))) {
        return SOP_RESULT_ERROR;
    }

    // Application descriptor
    Application applicationDescriptor = { 0 };

    if (!ContainsApplication(session, profile, profileDescriptor, applicationName, &applicationDescriptor)) {
        applicationDescriptor.version = 147464; // Calculated from the size of the descriptor
        applicationDescriptor.isPredefined = 0;
        memcpy(&applicationDescriptor.appName, &applicationName, 2048);

        // Add the current application to the new profile
        if ((CheckForError((*CreateApplication)(session, profile, &applicationDescriptor))) ||
            (CheckForError((*SaveSettings)(session)))) {
            return SOP_RESULT_ERROR;
        }
        else {
            result = SOP_RESULT_CHANGE;
        }
    }

    (*DestroySession)(session);
    session = 0;

    return result;
}

#endif

