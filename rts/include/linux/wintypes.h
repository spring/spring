#ifndef WINTYPES_H
#define WINTYPES_H

#include <stddef.h> // for wchar_t
#include <string.h>

#define XMD_H

typedef unsigned char Byte;
typedef short Int16;
typedef unsigned short UInt16;
typedef int Int32;
typedef unsigned int UInt32;
#ifdef _MSC_VER
typedef __int64 Int64;
typedef unsigned __int64 UInt64;
#else
typedef long long int Int64;
typedef unsigned long long int UInt64;
#endif

typedef char CHAR;
typedef unsigned char UCHAR;
typedef unsigned char BYTE;

typedef short SHORT;
typedef unsigned short USHORT;
typedef unsigned short WORD;
typedef short VARIANT_BOOL;

typedef int INT;
typedef int INT32;
typedef unsigned int UINT;
typedef unsigned int UINT32;
typedef INT32 LONG;   // LONG, ULONG and DWORD must be 32-bit
typedef UINT32 ULONG;
typedef UINT32 DWORD;

typedef Int64 LONGLONG;
typedef UInt64 ULONGLONG;

typedef struct LARGE_INTEGER { LONGLONG QuadPart; }LARGE_INTEGER;
typedef struct _ULARGE_INTEGER { ULONGLONG QuadPart;} ULARGE_INTEGER;

typedef const CHAR *LPCSTR;
typedef CHAR TCHAR;
typedef const TCHAR *LPCTSTR;
typedef wchar_t WCHAR;
typedef WCHAR OLECHAR;
typedef const WCHAR *LPCWSTR;
typedef OLECHAR *BSTR;
typedef const OLECHAR *LPCOLESTR;
typedef OLECHAR *LPOLESTR;

#define HRESULT LONG
#define FAILED(Status) ((HRESULT)(Status)<0)
typedef ULONG PROPID;
typedef LONG SCODE;

#define S_OK    ((HRESULT)0x00000000L)
#define S_FALSE ((HRESULT)0x00000001L)
#define E_NOTIMPL ((HRESULT)0x80004001L)
#define E_NOINTERFACE ((HRESULT)0x80004002L)
#define E_ABORT ((HRESULT)0x80004004L)
#define E_FAIL ((HRESULT)0x80004005L)
#define STG_E_INVALIDFUNCTION ((HRESULT)0x80030001L)
#define E_OUTOFMEMORY ((HRESULT)0x8007000EL)
#define E_INVALIDARG ((HRESULT)0x80070057L)

#endif
