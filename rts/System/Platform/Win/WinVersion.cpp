/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cassert>
#include <sstream>
#include <string>
#include <codecvt>
#include <locale> // std::wstring_convert

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x500
#endif

#include <windows.h>
#ifndef _MSC_VER
#include <ntdef.h>
#else
typedef _Return_type_success_(return >= 0) LONG NTSTATUS;
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif

#include "WinVersion.h"

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

#ifndef SM_SERVERR2
#define SM_SERVERR2 89
#endif


// from http://msdn.microsoft.com/en-us/library/ms724429(VS.85).aspx
// Windows version table mapping (as of november 2018) is as follows:
//   Windows 10               : 10.0*
//   Windows Server 2016      : 10.0*
//   Windows 8.1              :  6.3*
//   Windows Server 2012 R2   :  6.3*
//   Windows 8                :  6.2
//   Windows Server 2012      :  6.2
//   Windows 7                :  6.1
//   Windows Server 2008 R2   :  6.1
//   Windows Server 2008      :  6.0
//   Windows Vista            :  6.0
//   Windows Server 2003 R2   :  5.2
//   Windows Server 2003      :  5.2
//   Windows XP 64-Bit Edition:  5.2
//   Windows XP               :  5.1
//   Windows 2000             :  5.0
//
typedef void (WINAPI *PGNSI)(LPSYSTEM_INFO); // GetNativeSystemInfo
typedef LONG (WINAPI *PRTLGV)(OSVERSIONINFOW*); // RtlGetVersion
typedef BOOL (WINAPI *PGPI)(DWORD, DWORD, DWORD, DWORD, PDWORD); // GetProductInfo

std::string windows::GetDisplayString(bool getName, bool getVersion, bool getExtra)
{
	// need OSVERSIONINFO*EX*W for wSuiteMask etc
	OSVERSIONINFOEXW osvi;
	SYSTEM_INFO si;
	DWORD dwType;

	ZeroMemory(&si, sizeof(si));
	ZeroMemory(&osvi, sizeof(osvi));

	osvi.dwOSVersionInfoSize = sizeof(osvi);


	// use kernel call, GetVersionEx is deprecated in 8.1
	// and what it returns depends on application manifest
	PRTLGV pRTLGV = (PRTLGV) GetProcAddress(GetModuleHandle(TEXT("ntdll.dll")), "RtlGetVersion");

	if (pRTLGV == nullptr || !NT_SUCCESS(pRTLGV((OSVERSIONINFOW*) &osvi)))
		return "error getting Windows version";

	// RtlGetVersion exists since Windows 2000, which is ancient
	assert(osvi.dwMajorVersion >= 5 && osvi.dwPlatformId == VER_PLATFORM_WIN32_NT);


	// call GetNativeSystemInfo if supported for wProcessorArchitecture, GetSystemInfo otherwise
	PGNSI pGNSI = (PGNSI) GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), "GetNativeSystemInfo");

	if (pGNSI != nullptr)
		pGNSI(&si);
	else
		GetSystemInfo(&si);


	std::ostringstream oss;

	if (getName)
		oss << "Windows ";

	switch (osvi.dwMajorVersion) {
		case 10: {
			if (getName) {
				switch (osvi.dwMinorVersion) {
					case  0: { oss << ((osvi.wProductType == VER_NT_WORKSTATION)? "10": "Server 2016"); } break;
					default: {                                                                          } break;
				}
			}

			if (getVersion) {
				     if (osvi.dwBuildNumber >= 18267) { oss << ((osvi.wProductType == VER_NT_WORKSTATION)?   "TBA Insider Update": ""); } // 19H1
				else if (osvi.dwBuildNumber >= 17763) { oss << ((osvi.wProductType == VER_NT_WORKSTATION)?  "October 2018 Update": ""); } // Redstone 5
				else if (osvi.dwBuildNumber >= 17134) { oss << ((osvi.wProductType == VER_NT_WORKSTATION)?    "April 2018 Update": ""); } // Redstone 4
				else if (osvi.dwBuildNumber >= 16299) { oss << ((osvi.wProductType == VER_NT_WORKSTATION)? "Fall Creators Update": ""); } // Redstone 3
				else if (osvi.dwBuildNumber >= 15063) { oss << ((osvi.wProductType == VER_NT_WORKSTATION)?      "Creators Update": ""); } // Redstone 2
				else if (osvi.dwBuildNumber >= 14393) { oss << ((osvi.wProductType == VER_NT_WORKSTATION)?   "Anniversary Update": ""); } // Redstone 1
				else if (osvi.dwBuildNumber >= 10586) { oss << ((osvi.wProductType == VER_NT_WORKSTATION)?      "November Update": ""); } // Threshold 2
				else if (osvi.dwBuildNumber >= 10240) { oss << ((osvi.wProductType == VER_NT_WORKSTATION)?      "Initial Release": ""); } // Threshold 1
				else                                  { oss << ((osvi.wProductType == VER_NT_WORKSTATION)?      "Preview Release": ""); }
			}
		} break;

		case 6: {
			if (getName) {
				switch (osvi.dwMinorVersion) {
					case  3: { oss << ((osvi.wProductType == VER_NT_WORKSTATION)? "8.1"  : "Server 2012 R2"); } break;
					case  2: { oss << ((osvi.wProductType == VER_NT_WORKSTATION)? "8"    : "Server 2012"   ); } break;
					case  1: { oss << ((osvi.wProductType == VER_NT_WORKSTATION)? "7"    : "Server 2008 R2"); } break;
					case  0: { oss << ((osvi.wProductType == VER_NT_WORKSTATION)? "Vista": "Server 2008"   ); } break;
					default: {                                                                                } break;
				}
			}

			if (getVersion) {
				PGPI pGPI = (PGPI) GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), "GetProductInfo");
				pGPI(6, 0, 0, 0, &dwType);

				// FIXME: these might not apply to 6.2 and 6.3
				switch (dwType) {
					case PRODUCT_ULTIMATE                    : { oss << "Ultimate Edition"                            ; } break;
					case PRODUCT_HOME_PREMIUM                : { oss << "Home Premium Edition"                        ; } break;
					case PRODUCT_HOME_BASIC                  : { oss << "Home Basic Edition"                          ; } break;
					case PRODUCT_ENTERPRISE                  : { oss << "Enterprise Edition"                          ; } break;
					case PRODUCT_BUSINESS                    : { oss << "Business Edition"                            ; } break;
					case PRODUCT_STARTER                     : { oss << "Starter Edition"                             ; } break;
					case PRODUCT_CLUSTER_SERVER              : { oss << "Cluster Server Edition"                      ; } break;
					case PRODUCT_DATACENTER_SERVER           : { oss << "Datacenter Edition"                          ; } break;
					case PRODUCT_DATACENTER_SERVER_CORE      : { oss << "Datacenter Edition (core installation)"      ; } break;
					case PRODUCT_ENTERPRISE_SERVER           : { oss << "Enterprise Edition"                          ; } break;
					case PRODUCT_ENTERPRISE_SERVER_CORE      : { oss << "Enterprise Edition (core installation)"      ; } break;
					case PRODUCT_ENTERPRISE_SERVER_IA64      : { oss << "Enterprise Edition for Itanium-based Systems"; } break;
					case PRODUCT_SMALLBUSINESS_SERVER        : { oss << "Small Business Server"                       ; } break;
					// undocumented?
					// case PRODUCT_SMALLBUSINESS_SERVER_PREMIUM: { oss << "Small Business Server Premium Edition"       ; } break;
					case PRODUCT_STANDARD_SERVER             : { oss << "Standard Edition"                            ; } break;
					case PRODUCT_STANDARD_SERVER_CORE        : { oss << "Standard Edition (core installation)"        ; } break;
					case PRODUCT_WEB_SERVER                  : { oss << "Web Server Edition"                          ; } break;
					default                                  : {                                                        } break;
				}

				switch (si.wProcessorArchitecture) {
					case PROCESSOR_ARCHITECTURE_AMD64: { oss << ", 64-bit"; } break;
					case PROCESSOR_ARCHITECTURE_INTEL: { oss << ", 32-bit"; } break;
					default                          : {                    } break;
				}
			}
		} break;

		case 5: {
			switch (osvi.dwMinorVersion) {
				// Server 2003 or XP 64-bit
				case 2: {
					if (getName) {
						if (GetSystemMetrics(SM_SERVERR2))
							oss << "Server 2003 R2";
						else if (osvi.wSuiteMask == VER_SUITE_STORAGE_SERVER)
							oss << "Storage Server 2003";
						else if (osvi.wProductType == VER_NT_WORKSTATION && si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
							oss << "XP Professional x64 Edition";
						else
							oss << "Server 2003";
					}

					if (getVersion) {
						// test for the server type
						if (osvi.wProductType != VER_NT_WORKSTATION) {
							switch (si.wProcessorArchitecture) {
								case PROCESSOR_ARCHITECTURE_IA64: {
									if (osvi.wSuiteMask & VER_SUITE_DATACENTER)
										oss << "Datacenter Edition for Itanium-based Systems";
									else if (osvi.wSuiteMask & VER_SUITE_ENTERPRISE)
										oss << "Enterprise Edition for Itanium-based Systems";
								} break;
								case PROCESSOR_ARCHITECTURE_AMD64: {
									if (osvi.wSuiteMask & VER_SUITE_DATACENTER)
										oss << "Datacenter x64 Edition";
									else if (osvi.wSuiteMask & VER_SUITE_ENTERPRISE)
										oss << "Enterprise x64 Edition";
									else
										oss << "Standard x64 Edition";
								} break;
								default: {
									if (osvi.wSuiteMask & VER_SUITE_COMPUTE_SERVER)
										oss << "Compute Cluster Edition";
									else if (osvi.wSuiteMask & VER_SUITE_DATACENTER)
										oss << "Datacenter Edition";
									else if (osvi.wSuiteMask & VER_SUITE_ENTERPRISE)
										oss << "Enterprise Edition" ;
									else if (osvi.wSuiteMask & VER_SUITE_BLADE)
										oss << "Web Edition";
									else
										oss << "Standard Edition";
								} break;
							}
						}
					}
				} break;

				// XP
				case 1: {
					if (getName)
						oss << "XP";

					if (getVersion)
						oss << ((osvi.wSuiteMask & VER_SUITE_PERSONAL)? "Home Edition": "Professional");
				} break;

				// 2000
				case 0: {
					if (getName)
						oss << "2000";

					if (getVersion) {
						if (osvi.wProductType == VER_NT_WORKSTATION) {
							oss << "Professional";
						} else {
							if (osvi.wSuiteMask & VER_SUITE_DATACENTER)
								oss << "Datacenter Server";
							else if (osvi.wSuiteMask & VER_SUITE_ENTERPRISE)
								oss << "Advanced Server";
							else
								oss << "Server";
						}
					}
				} break;

				default: {
				} break;
			}
		} break;
	}

	if (getExtra) {
		// include build number and service pack (if any)
		oss << " (build " << osvi.dwBuildNumber;

		if (osvi.szCSDVersion[0] != 0) {
			static_assert(sizeof(wchar_t) >= sizeof(osvi.szCSDVersion[0]), "");

			// Windows uses UTF16
			std::wstring_convert<std::codecvt_utf16<wchar_t>, wchar_t> ws2s;

			const std::wstring wstr(osvi.szCSDVersion);
			const std::string nstr(ws2s.to_bytes(wstr));

			oss << ", " << nstr;
		}

		oss << ")";
	}

	return oss.str();
}


// this tries to read info about the CPU and available memory
std::string windows::GetHardwareString()
{
	std::ostringstream oss;

	unsigned char regbuf[200];
	DWORD regLength = sizeof(regbuf);
	DWORD regType = REG_SZ;
	HKEY regkey;

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Hardware\\Description\\System\\CentralProcessor\\0", 0, KEY_READ, &regkey) == ERROR_SUCCESS) {
		if (RegQueryValueEx(regkey, "ProcessorNameString", 0, &regType, regbuf, &regLength) == ERROR_SUCCESS) {
			oss << regbuf << "; ";
		} else {
			oss << "cannot read processor data; ";
		}

		RegCloseKey(regkey);
	} else {
		oss << "cannot open key with processor data; ";
	}

	MEMORYSTATUSEX statex;
	constexpr int div = 1024 * 1024;
	statex.dwLength = sizeof(statex);

	GlobalMemoryStatusEx(&statex);

	oss << (statex.ullTotalPhys / div) << "MB RAM, ";
	oss << (statex.ullTotalPageFile / div) << "MB pagefile";
	return oss.str();
}
