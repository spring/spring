///////////////////////////////////////////////////////////////////////////////
//
//  Module: WriteRegistry.cpp
//
//    Desc: Functions to write a registry hive to a file.
//
// Copyright (c) 2003 Grant McDorman
// This file is licensed using a BSD-type license:
//  This software is provided 'as-is', without any express or implied
//  warranty.  In no event will the authors be held liable for any damages
//  arising from the use of this software.
//
//  Permission is granted to anyone to use this software for any purpose,
//  including commercial applications, and to alter it and redistribute it
//  freely, subject to the following restrictions:
//
//  1. The origin of this software must not be misrepresented; you must not
//     claim that you wrote the original software. If you use this software
//     in a product, an acknowledgment in the product documentation would be
//     appreciated but is not required.
//  2. Altered source versions must be plainly marked as such, and must not be
//     misrepresented as being the original software.
//  3. This notice may not be removed or altered from any source distribution.
//
///////////////////////////////////////////////////////////////////////////////
#include <StdAfx.h>

#include <windows.h>    // CreateFile, WriteFile, CloseHandle, DeleteFile, Reg* functions
#include <stdio.h>	    // _snprintf

#include "WriteRegistry.h"

static bool WriteRegValue(HANDLE hFile, const char *key_path, const char *name, int name_len, DWORD type, const unsigned char *data, DWORD data_len);
static bool WriteValuesAndSubkeys(const char *key_path, HKEY parent_key, const char *subkey, HANDLE hFile);
static void WriteFileString(HANDLE hFile, const char *string);

bool WriteRegistryTreeToFile(const char *key, const char *filename)
{
	char *cp = strchr(key, '\\');
	if (cp == NULL) {
		return false;
	}
	int len = cp - key;
	HKEY hKey = 0;

#define IS_PATH(id, short_id) if (strncmp(key, #id, len) == 0 || strncmp(key, #short_id, len) == 0) hKey = id
    IS_PATH(HKEY_CLASSES_ROOT, HKCR);
    else IS_PATH(HKEY_CURRENT_USER, HKCU);
    else IS_PATH(HKEY_LOCAL_MACHINE, HKLM);
    else IS_PATH(HKEY_CURRENT_CONFIG, HKCC);
    else IS_PATH(HKEY_USERS, HKU);
    else IS_PATH(HKEY_PERFORMANCE_DATA, HKPD);
    else IS_PATH(HKEY_DYN_DATA, HKDD);
	else {
		return false;
	}
	return WriteRegistryTreeToFile(hKey, cp + 1, filename);
}

bool WriteRegistryTreeToFile(HKEY section, const char *subkey, const char *filename)
{
    bool status = false;
    HANDLE hFile = ::CreateFile(
                                filename,
                                GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                NULL,
                                CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL,
                                0);
    if (INVALID_HANDLE_VALUE != hFile) {
		char * key_path = "UNKNOWN";
#define SET_PATH(id) if (id == section) key_path = #id
        SET_PATH(HKEY_CLASSES_ROOT);
        else SET_PATH(HKEY_CURRENT_USER);
        else SET_PATH(HKEY_LOCAL_MACHINE);
        else SET_PATH(HKEY_CURRENT_CONFIG);
        else SET_PATH(HKEY_USERS);
        else SET_PATH(HKEY_PERFORMANCE_DATA);
        else SET_PATH(HKEY_DYN_DATA);
		WriteFileString(hFile, "REGEDIT4\r\n");
#undef SET_PATH
        try {
            status = WriteValuesAndSubkeys(key_path, section, subkey, hFile);
        } catch (...) {
            status = false;
        }
        CloseHandle(hFile);
        if (!status) {
            DeleteFile(filename);
        }
	}
    return status;
}

static bool WriteValuesAndSubkeys(const char *key_path, HKEY parent_key, const char *subkey, HANDLE hFile)
{
    HKEY key;

    if (RegOpenKeyEx(parent_key, subkey, 0, KEY_READ, &key) != ERROR_SUCCESS) {
		OutputDebugString("RegOpenKeyEx failed, key:\n");
		OutputDebugString(subkey);
        return false;
    }
    DWORD num_subkeys;
    DWORD max_subkey_len;
    DWORD num_values;
    DWORD max_name_len;
    DWORD max_value_len;
    DWORD max_id_len;

    if (RegQueryInfoKey(key,
						NULL, // class
						NULL, // num_class
						NULL, // reserved
                        &num_subkeys, &max_subkey_len,
						NULL, // MaxClassLen
						&num_values, &max_name_len, &max_value_len, NULL, NULL) != ERROR_SUCCESS) {
		OutputDebugString("RegQueryInfoKey failed, key:\n");
		OutputDebugString(subkey);
        return false;
    }

    max_id_len = (max_name_len > max_subkey_len) ? max_name_len : max_subkey_len;
    char *this_path = reinterpret_cast<char *>(alloca(strlen(key_path) + strlen(subkey) + 2));
    // strcpy/strcat safe because of above alloca
    strcpy(this_path, key_path);
	strcat(this_path, "\\");
    strcat(this_path, subkey);

    WriteFileString(hFile, "\r\n[");
    WriteFileString(hFile, this_path);
    WriteFileString(hFile, "]\r\n");

    // enumerate values
    char *name = reinterpret_cast<char *>(alloca(max_id_len*2 + 2));
    unsigned char *data = reinterpret_cast<unsigned char *>(alloca(max_value_len*2 + 2));
    DWORD index;
    bool status = true;

    for (index = 0; index < num_values && status; index++) {
        DWORD name_len = max_id_len + 1;
        DWORD value_len = max_value_len + 1;
	    DWORD type;
        if (RegEnumValue(key, index, name, &name_len, NULL, &type, data, &value_len) == ERROR_SUCCESS) {
            status = WriteRegValue(hFile, this_path, name, name_len, type, data, value_len);
        }
    }

    // enumerate subkeys
    for (index = 0; index < num_subkeys && status; index++) {
        DWORD name_len = max_id_len + 1;
        if (RegEnumKeyEx(key, index, name, &name_len, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
            status = WriteValuesAndSubkeys(this_path, key, name, hFile);
        }
    }

    RegCloseKey(key);

    return status;
}

static bool WriteRegValue(HANDLE hFile, const char * /*key_path*/, const char *name, int /* name_len */, DWORD type, const unsigned char *data, DWORD data_len)
{
	WriteFileString(hFile, "\"");
    WriteFileString(hFile, name);

    char string_type[64];

    switch(type) {
     case REG_DWORD:  // A 32-bit number.
        strncpy(string_type, "\"=dword:", sizeof string_type);
        break;

     case REG_SZ: // A null terminated string.
        strncpy(string_type, "\"=\"", sizeof string_type);
        break;

     case REG_BINARY: // Binary data in any form.
        strncpy(string_type, "\"=hex:", sizeof string_type);
        break;

     case REG_EXPAND_SZ: // A null-terminated string that contains unexpanded references to environment variables (for example, "%PATH%"). It will be a Unicode or ANSI string depending on whether you use the Unicode or ANSI functions. To expand the environment variable references, use the ExpandEnvironmentStrings function.
     case REG_LINK: // A Unicode symbolic link. Used internally; applications should not use this type.
     case REG_MULTI_SZ: // An array of null-terminated strings, terminated by two null characters. 
     case REG_NONE: // No defined value type.
     case REG_DWORD_BIG_ENDIAN: // A 64-bit number in big-endian format.
     case REG_RESOURCE_LIST: // A device-driver resource list.
     default:
        _snprintf(string_type, sizeof string_type, "\"=hex(%x):", type);
        break;
    }

    WriteFileString(hFile, string_type);

    if (type == REG_SZ || type == REG_EXPAND_SZ) {
        // escape special characters; length includes trailing NUL
        int i;
		// don't crash'n'burn if data_len is 0
        for (i = 0; i < static_cast<int>(data_len) - 1; i++) {
            if (data[i] == '\\' || data[i] == '"') {
                WriteFileString(hFile, "\\");
            }
            if (isprint(data[i])) {
				DWORD written;
                if (!WriteFile(hFile, &data[i], 1, &written, NULL) || written != 1) {
                    return false;
                }
            } else {
                _snprintf(string_type, sizeof string_type, "\\%02x", data[i]);
                WriteFileString(hFile, string_type);
            }
        }
		WriteFileString(hFile, "\"");
    } else if (type == REG_DWORD) {
        // write as hex, MSB first
        int i;
        for (i = static_cast<int>(data_len) - 1; i >= 0; i--) {
            _snprintf(string_type, sizeof string_type, "%02x", data[i]);
            WriteFileString(hFile, string_type);
        }
    } else {
        // write as comma-separated hex values
        DWORD i;
        for (i = 0; i < data_len; i++) {
            _snprintf(string_type, sizeof string_type, "%s%02x", i > 0 ? "," : "", data[i]);
            WriteFileString(hFile, string_type);
            if (i > 0 && i % 16 == 0) {
                WriteFileString(hFile, "\r\n");
            }
        }
    }
    WriteFileString(hFile, "\r\n");

	return true;
}

                  
                
static void WriteFileString(HANDLE hFile, const char *string)
{
    DWORD written;
    if (!WriteFile(hFile, string, strlen(string), &written, NULL) || written != strlen(string)) {
		OutputDebugString("WriteFile failed\n");
        throw false;
    }
}
