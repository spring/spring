/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <sstream>
#include <string>

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x500
#endif

#include <windows.h>

#ifndef PRODUCT_BUSINESS

#define PRODUCT_BUSINESS 0x00000006
#define PRODUCT_BUSINESS_N 0x00000010
#define PRODUCT_CLUSTER_SERVER 0x00000012
#define PRODUCT_DATACENTER_SERVER 0x00000008
#define PRODUCT_DATACENTER_SERVER_CORE 0x0000000C
#define PRODUCT_DATACENTER_SERVER_CORE_V 0x00000027
#define PRODUCT_DATACENTER_SERVER_V 0x00000025
#define PRODUCT_ENTERPRISE 0x00000004
#define PRODUCT_ENTERPRISE_N 0x0000001B
#define PRODUCT_ENTERPRISE_SERVER 0x0000000A
#define PRODUCT_ENTERPRISE_SERVER_CORE 0x0000000E
#define PRODUCT_ENTERPRISE_SERVER_CORE_V 0x00000029
#define PRODUCT_ENTERPRISE_SERVER_IA64 0x0000000F
#define PRODUCT_ENTERPRISE_SERVER_V 0x00000026
#define PRODUCT_HOME_BASIC 0x00000002
#define PRODUCT_HOME_BASIC_N 0x00000005
#define PRODUCT_HOME_PREMIUM 0x00000003
#define PRODUCT_HOME_PREMIUM_N 0x0000001A
#define PRODUCT_HOME_SERVER 0x00000013
#define PRODUCT_HYPERV 0x0000002A
#define PRODUCT_MEDIUMBUSINESS_SERVER_MANAGEMENT 0x0000001E
#define PRODUCT_MEDIUMBUSINESS_SERVER_MESSAGING 0x00000020
#define PRODUCT_MEDIUMBUSINESS_SERVER_SECURITY 0x0000001F
#define PRODUCT_SERVER_FOR_SMALLBUSINESS 0x00000018
#define PRODUCT_SERVER_FOR_SMALLBUSINESS_V 0x00000023
#define PRODUCT_SMALLBUSINESS_SERVER 0x00000009
#define PRODUCT_STANDARD_SERVER 0x00000007
#define PRODUCT_STANDARD_SERVER_CORE 0x0000000D
#define PRODUCT_STANDARD_SERVER_CORE_V 0x00000028
#define PRODUCT_STANDARD_SERVER_V 0x00000024
#define PRODUCT_STARTER 0x0000000B
#define PRODUCT_STORAGE_ENTERPRISE_SERVER 0x00000017
#define PRODUCT_STORAGE_EXPRESS_SERVER 0x00000014
#define PRODUCT_STORAGE_STANDARD_SERVER 0x00000015
#define PRODUCT_STORAGE_WORKGROUP_SERVER 0x00000016
#define PRODUCT_UNDEFINED 0x00000000
#define PRODUCT_ULTIMATE 0x00000001
#define PRODUCT_ULTIMATE_N 0x0000001C
#define PRODUCT_WEB_SERVER 0x00000011
#define PRODUCT_WEB_SERVER_CORE 0x0000001D

#endif

typedef void (WINAPI *PGNSI)(LPSYSTEM_INFO);
typedef BOOL (WINAPI *PGPI)(DWORD, DWORD, DWORD, DWORD, PDWORD);

#ifndef SM_SERVERR2
#define SM_SERVERR2 89
#endif

// this is a modified version of http://msdn.microsoft.com/en-us/library/ms724429(VS.85).aspx
// always provide a long enough buffer
std::string GetOSDisplayString()
{
    OSVERSIONINFOEX osvi;
    SYSTEM_INFO si;
    PGNSI pGNSI;
    BOOL bOsVersionInfoEx;
    DWORD dwType;

    ZeroMemory(&si, sizeof(SYSTEM_INFO));
    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));

    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

    if ( !(bOsVersionInfoEx = GetVersionEx ((OSVERSIONINFO *) &osvi)) )
		return std::string("error getting Windows version");

    // Call GetNativeSystemInfo if supported or GetSystemInfo otherwise.

    pGNSI = (PGNSI) GetProcAddress(
                GetModuleHandle(TEXT("kernel32.dll")),
                "GetNativeSystemInfo");
    if (NULL != pGNSI)
        pGNSI(&si);
    else GetSystemInfo(&si);

    if ( VER_PLATFORM_WIN32_NT==osvi.dwPlatformId &&
            osvi.dwMajorVersion > 4 )
    {
		std::ostringstream oss;
        oss << "Microsoft ";

        // Test for the specific product.

        if ( osvi.dwMajorVersion == 6)
        {
            if (osvi.dwMinorVersion == 0) {
                if ( osvi.wProductType == VER_NT_WORKSTATION )
                    oss << "Windows Vista ";
                else oss << "Windows Server 2008 ";
            } else if (osvi.dwMinorVersion == 1) {
               if( osvi.wProductType == VER_NT_WORKSTATION )
                   oss << "Windows 7 ";
               else oss << "Windows Server 2008 R2 ";
            }

            PGPI pGPI = (PGPI) GetProcAddress(
                       GetModuleHandle(TEXT("kernel32.dll")),
                       "GetProductInfo");

            pGPI( 6, 0, 0, 0, &dwType);

            switch ( dwType )
            {
            case PRODUCT_ULTIMATE:
                oss << "Ultimate Edition";
                break;
            case PRODUCT_HOME_PREMIUM:
                oss << "Home Premium Edition";
                break;
            case PRODUCT_HOME_BASIC:
                oss << "Home Basic Edition";
                break;
            case PRODUCT_ENTERPRISE:
                oss << "Enterprise Edition";
                break;
            case PRODUCT_BUSINESS:
                oss << "Business Edition";
                break;
            case PRODUCT_STARTER:
                oss << "Starter Edition";
                break;
            case PRODUCT_CLUSTER_SERVER:
                oss << "Cluster Server Edition";
                break;
            case PRODUCT_DATACENTER_SERVER:
                oss << "Datacenter Edition";
                break;
            case PRODUCT_DATACENTER_SERVER_CORE:
                oss << "Datacenter Edition (core installation)";
                break;
            case PRODUCT_ENTERPRISE_SERVER:
                oss << "Enterprise Edition";
                break;
            case PRODUCT_ENTERPRISE_SERVER_CORE:
                oss << "Enterprise Edition (core installation)";
                break;
            case PRODUCT_ENTERPRISE_SERVER_IA64:
                oss << "Enterprise Edition for Itanium-based Systems";
                break;
            case PRODUCT_SMALLBUSINESS_SERVER:
                oss << "Small Business Server";
                break;
                /* 	undocumented?
                case PRODUCT_SMALLBUSINESS_SERVER_PREMIUM:
                oss << "Small Business Server Premium Edition";
                break; */
            case PRODUCT_STANDARD_SERVER:
                oss << "Standard Edition";
                break;
            case PRODUCT_STANDARD_SERVER_CORE:
                oss << "Standard Edition (core installation)";
                break;
            case PRODUCT_WEB_SERVER:
                oss << "Web Server Edition";
                break;
            }
            if ( si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64 )
                oss <<  ", 64-bit";
            else if (si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_INTEL )
                oss << ", 32-bit";
        }

        if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2 )
        {
            if ( GetSystemMetrics(SM_SERVERR2) )
                oss <<  "Windows Server 2003 R2, ";
            else if ( osvi.wSuiteMask==VER_SUITE_STORAGE_SERVER )
                oss <<  "Windows Storage Server 2003";
            else if ( osvi.wProductType == VER_NT_WORKSTATION &&
                      si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64)
            {
                oss <<  "Windows XP Professional x64 Edition";
            }
            else oss << "Windows Server 2003, ";

            // Test for the server type.
            if ( osvi.wProductType != VER_NT_WORKSTATION )
            {
                if ( si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_IA64 )
                {
                    if ( osvi.wSuiteMask & VER_SUITE_DATACENTER )
                        oss <<  "Datacenter Edition for Itanium-based Systems";
                    else if ( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
                        oss <<  "Enterprise Edition for Itanium-based Systems";
                }

                else if ( si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64 )
                {
                    if ( osvi.wSuiteMask & VER_SUITE_DATACENTER )
                        oss <<  "Datacenter x64 Edition";
                    else if ( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
                        oss <<  "Enterprise x64 Edition";
                    else oss <<  "Standard x64 Edition";
                }

                else
                {
                    if ( osvi.wSuiteMask & VER_SUITE_COMPUTE_SERVER )
                        oss <<  "Compute Cluster Edition";
                    else if ( osvi.wSuiteMask & VER_SUITE_DATACENTER )
                        oss <<  "Datacenter Edition";
                    else if ( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
                        oss <<  "Enterprise Edition" ;
                    else if ( osvi.wSuiteMask & VER_SUITE_BLADE )
                        oss <<  "Web Edition";
                    else oss <<  "Standard Edition";
                }
            }
        }

        if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1 )
        {
            oss << "Windows XP ";
            if ( osvi.wSuiteMask & VER_SUITE_PERSONAL )
                oss <<  "Home Edition";
            else oss <<  "Professional";
        }

        if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0 )
        {
            oss << "Windows 2000 ";

            if ( osvi.wProductType == VER_NT_WORKSTATION )
            {
                oss <<  "Professional";
            }
            else
            {
                if ( osvi.wSuiteMask & VER_SUITE_DATACENTER )
                    oss <<  "Datacenter Server";
                else if ( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
                    oss <<  "Advanced Server";
                else oss <<  "Server";
            }
        }

        // Include service pack (if any) and build number.

        if (strlen(osvi.szCSDVersion) > 0 )
        {
            oss << " " << osvi.szCSDVersion;
        }

        oss << " (build " << osvi.dwBuildNumber << ")";

        return oss.str();
    }

    else
    {
        return std::string("unsupported version of Windows");
    }
}


// this tries to read info about the CPU and available memory
std::string GetHardwareInfoString()
{
	std::ostringstream oss;

    unsigned char regbuf[200];
    DWORD regLength=sizeof(regbuf);
    DWORD regType=REG_SZ;
    HKEY regkey;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     "Hardware\\Description\\System\\CentralProcessor\\0",
                     0, KEY_READ, &regkey)==ERROR_SUCCESS)
    {
        if (RegQueryValueEx(regkey,"ProcessorNameString",0,&regType,regbuf,&regLength)==ERROR_SUCCESS)
        {
            oss << regbuf << "; ";
        }
        else
        {
            oss << "cannot read processor data; ";
        }
        RegCloseKey(regkey);
    }
    else
    {
        oss << "cannot open key with processor data; ";
    }

    MEMORYSTATUSEX statex;
    const int div = 1024*1024;
    statex.dwLength = sizeof (statex);

    GlobalMemoryStatusEx (&statex);

    oss << (statex.ullTotalPhys/div) << "MB RAM, "
    << (statex.ullTotalPageFile/div) << "MB pagefile";
    return oss.str();
}
