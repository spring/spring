///////////////////////////////////////////////////////////////////////////////
//
//  Module: excprpt.cpp
//
//    Desc: See excprpt.h
//
// Copyright (c) 2003 Michael Carruth
//
///////////////////////////////////////////////////////////////////////////////


#include "stdafx.h"
#include "excprpt.h"
#include "utility.h"

#include "StackTrace.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


//-----------------------------------------------------------------------------
// CExceptionReport::CExceptionReport
//
// 
//
CExceptionReport::CExceptionReport(PEXCEPTION_POINTERS ExceptionInfo, BSTR message)
{
   m_excpInfo = ExceptionInfo;
   m_message = message;
   TCHAR szModName[_MAX_FNAME + 1];
   GetModuleFileName(NULL, szModName, _MAX_FNAME);
   m_sModule = szModName;
}


//-----------------------------------------------------------------------------
// CExceptionReport::getCrashFile
//
// Creates the dump file returning the file name
//
CString CExceptionReport::getCrashFile()
{
   CString sFile;

   // Create the dump file name
   sFile.Format(_T("%s\\%s.dmp"), getenv("TEMP"), CUtility::getAppName());

   // Create the file
   HANDLE hFile = CreateFile(
      sFile,
      GENERIC_WRITE,
      0,
      NULL,
      CREATE_ALWAYS,
      FILE_ATTRIBUTE_NORMAL,
      NULL);

   //
   // Write the minidump to the file
   //
   if (hFile)
   {
      writeDumpFile(hFile, m_excpInfo, reinterpret_cast<void *>(this));
   }

   // Close file
   CloseHandle(hFile);

   return sFile;
}


void CExceptionReport::writeDumpFile(HANDLE hFile, PEXCEPTION_POINTERS excpInfo, void *data)
{
	if (excpInfo == NULL) {
      // Generate exception to get proper context in dump
	   __try {
		   RaiseException(EXCEPTION_BREAKPOINT, 0, 0, NULL);
	   } __except(writeDumpFile(hFile, GetExceptionInformation(), data), EXCEPTION_CONTINUE_EXECUTION) {
	   }
	} else {
      MINIDUMP_EXCEPTION_INFORMATION eInfo;
      eInfo.ThreadId = GetCurrentThreadId();
      eInfo.ExceptionPointers = excpInfo;
      eInfo.ClientPointers = FALSE;

      MINIDUMP_CALLBACK_INFORMATION cbMiniDump;
      cbMiniDump.CallbackRoutine = CExceptionReport::miniDumpCallback;
      cbMiniDump.CallbackParam = data;

      MiniDumpWriteDump(
         GetCurrentProcess(),
         GetCurrentProcessId(),
         hFile,
         MiniDumpWithIndirectlyReferencedMemory,
         excpInfo ? &eInfo : NULL,
		 NULL,
         &cbMiniDump);
   }
}

//-----------------------------------------------------------------------------
// CExceptionReport::getCrashLog
//
// Creates the XML log file returning the name
//
CString CExceptionReport::getCrashLog()
{
   CString sFile;
   MSXML::IXMLDOMDocument *pDoc  = NULL;
   MSXML::IXMLDOMNode *root      = NULL;
   MSXML::IXMLDOMNode *node      = NULL;
   MSXML::IXMLDOMNode *newNode   = NULL;
   BSTR rootName = ::SysAllocString(L"Exception");
   VARIANT v;

   CoInitialize(NULL);

   // Create an empty XML document
   CHECKHR(CoCreateInstance(
      MSXML::CLSID_DOMDocument, 
      NULL, 
      CLSCTX_INPROC_SERVER,
      MSXML::IID_IXMLDOMDocument, 
      (void**)&pDoc));

   // Create root node
   root = CreateDOMNode(pDoc, MSXML::NODE_ELEMENT, rootName);

   //
   // Add exception record node
   //
   if (m_excpInfo)
   {
      node = CreateExceptionRecordNode(pDoc, m_excpInfo->ExceptionRecord);
      CHECKHR(root->appendChild(node, &newNode));
      // The XML Document should now own the node.
      SAFERELEASE(node);
      SAFERELEASE(newNode);
   }

   //
   // Add optional message node
   //
   if (m_message != NULL) {
	   node = CreateMsgNode(pDoc, m_message);
	   CHECKHR(root->appendChild(node, &newNode));
	   // The XML Document should now own the node.
	   SAFERELEASE(node);
	   SAFERELEASE(newNode);
   }

   //
   // Add processor node
   //
   node = CreateProcessorNode(pDoc);
   CHECKHR(root->appendChild(node, &newNode));
   // The XML Document should now own the node.
   SAFERELEASE(node);
   SAFERELEASE(newNode);

   //
   // Add OS node
   //
   node = CreateOSNode(pDoc);
   CHECKHR(root->appendChild(node, &newNode));
   // The XML Document should now own the node.
   SAFERELEASE(node);
   SAFERELEASE(newNode);

   //
   // Add modules node
   //
   node = CreateModulesNode(pDoc);
   CHECKHR(root->appendChild(node, &newNode));
   // The XML Document should now own the node.
   SAFERELEASE(node);
   SAFERELEASE(newNode);

   //
   // Add stack walkback node
   //
   node = CreateWalkbackNode(pDoc, m_excpInfo != NULL ? m_excpInfo->ContextRecord : NULL);
   CHECKHR(root->appendChild(node, &newNode));
   // The XML Document should now own the node.
   SAFERELEASE(node);
   SAFERELEASE(newNode);


   // Add the root to the doc
   CHECKHR(pDoc->appendChild(root, NULL));

   //
   // Create dat file name and save
   //
   sFile.Format(_T("%s\\%s.xml"), getenv("TEMP"), CUtility::getAppName());
   V_VT(&v) = VT_BSTR;
   V_BSTR(&v) = sFile.AllocSysString();
   pDoc->save(v);
   SysFreeString(V_BSTR(&v));

CleanUp:
   SAFERELEASE(pDoc);
   SAFERELEASE(root);
   SAFERELEASE(node);
   SAFERELEASE(newNode);
   SysFreeString(rootName);

   CoUninitialize();

   return sFile;
}


//-----------------------------------------------------------------------------
// CExceptionReport::getNumSymbolFiles
//
// Returns the number of symbols files found
//
int CExceptionReport::getNumSymbolFiles()
{
   return m_symFiles.GetSize();
}


//-----------------------------------------------------------------------------
// CExceptionReport::getSymbolFile
//
// Returns the symbol file name given an index
//
CString CExceptionReport::getSymbolFile(int index)
{
   CString ret;

   if (0 < index && index < m_symFiles.GetSize())
      ret = m_symFiles[index];

   return ret;
}

//-----------------------------------------------------------------------------
// CExceptionReport::CreateDOMNode
//
// Helper function 
//
MSXML::IXMLDOMNode*
CExceptionReport::CreateDOMNode(MSXML::IXMLDOMDocument* pDoc, 
                                int type, 
                                BSTR bstrName)
{
    MSXML::IXMLDOMNode * node;
    VARIANT vtype;

    vtype.vt = VT_I4;
    V_I4(&vtype) = (int)type;

    pDoc->createNode(vtype, bstrName, NULL, &node);
    return node;
}

//-----------------------------------------------------------------------------
// CreateExceptionSymbolAttributes
//
// Create attributes in the exception record with the symbolic info, if available
//
void  CExceptionReport::CreateExceptionSymbolAttributes(DWORD /*address*/, const char * /*ImageName*/,
									  const char *FunctionName, DWORD functionDisp,
									  const char *Filename, DWORD LineNumber, DWORD lineDisp,
									  void *data)
{
   CString sAddr;
   BSTR funcName					= ::SysAllocString(L"FunctionName");
   BSTR funcDispName				= ::SysAllocString(L"FunctionDisplacement");
   BSTR fileName					= ::SysAllocString(L"Filename");
   BSTR lineName					= ::SysAllocString(L"LineNumber");
   BSTR lineDispName				= ::SysAllocString(L"LineDisplacement");
   CExceptionReport	*self = reinterpret_cast<CExceptionReport *>(data);

   VARIANT v;

   // don't need ImageName [module], as that is already done
   if (FunctionName != NULL) {
		V_VT(&v) = VT_BSTR;
		V_BSTR(&v) = CString(FunctionName).AllocSysString();
		self->m_exception_element->setAttribute(funcName, v);
		// Recycle variant
		SysFreeString(V_BSTR(&v));
		sAddr.Format(_T("0x%x"), functionDisp);
		V_VT(&v) = VT_BSTR;
		V_BSTR(&v) = sAddr.AllocSysString();
		self->m_exception_element->setAttribute(funcDispName, v);
		// Recycle variant
		SysFreeString(V_BSTR(&v));
   }

   if (Filename != NULL) {
		V_VT(&v) = VT_BSTR;
		V_BSTR(&v) = CString(Filename).AllocSysString();
		self->m_exception_element->setAttribute(fileName, v);
		// Recycle variant
		SysFreeString(V_BSTR(&v));

		sAddr.Format(_T("%d"), LineNumber);
		V_VT(&v) = VT_BSTR;
		V_BSTR(&v) = sAddr.AllocSysString();
		self->m_exception_element->setAttribute(lineName, v);
		// Recycle variant
		SysFreeString(V_BSTR(&v));

		sAddr.Format(_T("0x%x"), lineDisp);
		V_VT(&v) = VT_BSTR;
		V_BSTR(&v) = sAddr.AllocSysString();
		self->m_exception_element->setAttribute(lineDispName, v);
		// Recycle variant
		SysFreeString(V_BSTR(&v));
   }
   ::SysFreeString(funcName);
   ::SysFreeString(funcDispName);
   ::SysFreeString(fileName);
   ::SysFreeString(lineName);
   ::SysFreeString(lineDispName);
}

//-----------------------------------------------------------------------------
// CExceptionReport::CreateExceptionRecordNode
//
//
//
MSXML::IXMLDOMNode*
CExceptionReport::CreateExceptionRecordNode(MSXML::IXMLDOMDocument* pDoc, 
                                            EXCEPTION_RECORD* pExceptionRecord)
{
   MSXML::IXMLDOMNode*     pNode    = NULL;
   MSXML::IXMLDOMElement*  pElement = NULL;
   BSTR nodeName                    = ::SysAllocString(L"ExceptionRecord");
   BSTR modName                     = ::SysAllocString(L"ModuleName");
   BSTR codeName                    = ::SysAllocString(L"ExceptionCode");
   BSTR descName                    = ::SysAllocString(L"ExceptionDescription");
   BSTR addrName                    = ::SysAllocString(L"ExceptionAddress");
   VARIANT v;
   CString sAddr;

   // Create exception record node
   pNode = CreateDOMNode(pDoc, MSXML::NODE_ELEMENT, nodeName);

   // Get element interface
   CHECKHR(pNode->QueryInterface(MSXML::IID_IXMLDOMElement, (void**)&pElement));

   //
   // Set module name attribute
   //
   V_VT(&v)    = VT_BSTR;
   V_BSTR(&v)  = A2BSTR(m_sModule);
   pElement->setAttribute(modName, v);
   // Recycle variant
   SysFreeString(V_BSTR(&v));

   //
   // Set exception code
   //
   sAddr.Format(_T("%#x"), pExceptionRecord->ExceptionCode);
   m_sException = sAddr;
   V_VT(&v)    = VT_BSTR;
   V_BSTR(&v)  = sAddr.AllocSysString();
   pElement->setAttribute(codeName, v);
   // Recycle variant
   SysFreeString(V_BSTR(&v));

   //
   // Set exception description
   //
   V_VT(&v)    = VT_BSTR;
   switch (pExceptionRecord->ExceptionCode)
   {
   case EXCEPTION_ACCESS_VIOLATION:
      V_BSTR(&v) = ::SysAllocString(L"EXCEPTION_ACCESS_VIOLATION");
      break;
   case EXCEPTION_DATATYPE_MISALIGNMENT:
      V_BSTR(&v) = ::SysAllocString(L"EXCEPTION_DATATYPE_MISALIGNMENT");
      break;
   case EXCEPTION_BREAKPOINT:
      V_BSTR(&v) = ::SysAllocString(L"EXCEPTION_BREAKPOINT");
      break;
   case EXCEPTION_SINGLE_STEP:
      V_BSTR(&v) = ::SysAllocString(L"EXCEPTION_SINGLE_STEP");
      break;
   case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
      V_BSTR(&v) = ::SysAllocString(L"EXCEPTION_ARRAY_BOUNDS_EXCEEDED");
      break;
   case EXCEPTION_FLT_DENORMAL_OPERAND:
      V_BSTR(&v) = ::SysAllocString(L"EXCEPTION_FLT_DENORMAL_OPERAND");
      break;
   case EXCEPTION_FLT_DIVIDE_BY_ZERO:
      V_BSTR(&v) = ::SysAllocString(L"EXCEPTION_FLT_DIVIDE_BY_ZERO");
      break;
   case EXCEPTION_FLT_INEXACT_RESULT:
      V_BSTR(&v) = ::SysAllocString(L"EXCEPTION_FLT_INEXACT_RESULT");
      break;
   case EXCEPTION_FLT_INVALID_OPERATION:
      V_BSTR(&v) = ::SysAllocString(L"EXCEPTION_FLT_INVALID_OPERATION");
      break;
   case EXCEPTION_FLT_OVERFLOW:
      V_BSTR(&v) = ::SysAllocString(L"EXCEPTION_FLT_OVERFLOW");
      break;
   case EXCEPTION_FLT_STACK_CHECK:
      V_BSTR(&v) = ::SysAllocString(L"EXCEPTION_FLT_STACK_CHECK");
      break;
   case EXCEPTION_FLT_UNDERFLOW:
      V_BSTR(&v) = ::SysAllocString(L"EXCEPTION_FLT_UNDERFLOW");
      break;
   case EXCEPTION_INT_DIVIDE_BY_ZERO:
      V_BSTR(&v) = ::SysAllocString(L"EXCEPTION_INT_DIVIDE_BY_ZERO");
      break;
   case EXCEPTION_INT_OVERFLOW:
      V_BSTR(&v) = ::SysAllocString(L"EXCEPTION_INT_OVERFLOW");
      break;
   case EXCEPTION_PRIV_INSTRUCTION:
      V_BSTR(&v) = ::SysAllocString(L"EXCEPTION_PRIV_INSTRUCTION");
      break;
   case EXCEPTION_IN_PAGE_ERROR:
      V_BSTR(&v) = ::SysAllocString(L"EXCEPTION_IN_PAGE_ERROR");
      break;
   case EXCEPTION_ILLEGAL_INSTRUCTION:
      V_BSTR(&v) = ::SysAllocString(L"EXCEPTION_ILLEGAL_INSTRUCTION");
      break;
   case EXCEPTION_NONCONTINUABLE_EXCEPTION:
      V_BSTR(&v) = ::SysAllocString(L"EXCEPTION_NONCONTINUABLE_EXCEPTION");
      break;
   case EXCEPTION_STACK_OVERFLOW:
      V_BSTR(&v) = ::SysAllocString(L"EXCEPTION_STACK_OVERFLOW");
      break;
   case EXCEPTION_INVALID_DISPOSITION:
      V_BSTR(&v) = ::SysAllocString(L"EXCEPTION_INVALID_DISPOSITION");
      break;
   case EXCEPTION_GUARD_PAGE:
      V_BSTR(&v) = ::SysAllocString(L"EXCEPTION_GUARD_PAGE");
      break;
   case EXCEPTION_INVALID_HANDLE:
      V_BSTR(&v) = ::SysAllocString(L"EXCEPTION_INVALID_HANDLE");
      break;
   default:
      V_BSTR(&v) = L"EXCEPTION_UNKNOWN";
      break;
   }
   pElement->setAttribute(descName, v);
   // Recycle variant
   SysFreeString(V_BSTR(&v));

   //
   // Set exception address
   //
   sAddr.Format(_T("%#x"), pExceptionRecord->ExceptionAddress);
   m_sAddress = sAddr;
   V_VT(&v)    = VT_BSTR;
   V_BSTR(&v)  = sAddr.AllocSysString();
   pElement->setAttribute(addrName, v);
   // Recycle variant
   SysFreeString(V_BSTR(&v));

   // Try to include symbolic information
   m_exception_element = pElement;
   AddressToSymbol(reinterpret_cast<DWORD>(pExceptionRecord->ExceptionAddress)-1,
	   CreateExceptionSymbolAttributes,
	   reinterpret_cast<void *>(this));
CleanUp:
   ::SysFreeString(nodeName);
   ::SysFreeString(modName);
   ::SysFreeString(codeName);
   ::SysFreeString(addrName);
   SAFERELEASE(pElement);

   return pNode;
}

//-----------------------------------------------------------------------------
// CExceptionReport::CreateProcessorNode
//
//
//
MSXML::IXMLDOMNode*
CExceptionReport::CreateProcessorNode(MSXML::IXMLDOMDocument* pDoc)
{
   MSXML::IXMLDOMNode*     pNode    = NULL;
   MSXML::IXMLDOMElement*  pElement = NULL;
   BSTR nodeName                    = ::SysAllocString(L"Processor");
   BSTR archName                    = ::SysAllocString(L"Architecture");
   BSTR levelName                   = ::SysAllocString(L"Level");
   BSTR numberName                  = ::SysAllocString(L"NumberOfProcessors");
   SYSTEM_INFO si;
   VARIANT v;

   // Create exception record node
   pNode = CreateDOMNode(pDoc, MSXML::NODE_ELEMENT, nodeName);

   // Get element interface
   CHECKHR(pNode->QueryInterface(MSXML::IID_IXMLDOMElement, (void**)&pElement));

   //
   // Get processor info
   //
   GetSystemInfo(&si);

   //
   // Set architecture
   //
   V_VT(&v) = VT_BSTR;
   switch (si.wProcessorArchitecture)
   {
   case PROCESSOR_ARCHITECTURE_INTEL:
      V_BSTR(&v) = ::SysAllocString(L"PROCESSOR_ARCHITECTURE_INTEL");
      break;
   case PROCESSOR_ARCHITECTURE_MIPS:
      V_BSTR(&v) = ::SysAllocString(L"PROCESSOR_ARCHITECTURE_MIPS");
      break;
   case PROCESSOR_ARCHITECTURE_ALPHA:
      V_BSTR(&v) = ::SysAllocString(L"PROCESSOR_ARCHITECTURE_ALPHA");
      break;
   case PROCESSOR_ARCHITECTURE_PPC:
      V_BSTR(&v) = ::SysAllocString(L"PROCESSOR_ARCHITECTURE_PPC");
      break;
   case PROCESSOR_ARCHITECTURE_SHX:
      V_BSTR(&v) = ::SysAllocString(L"PROCESSOR_ARCHITECTURE_SHX");
      break;
   case PROCESSOR_ARCHITECTURE_ARM:
      V_BSTR(&v) = ::SysAllocString(L"PROCESSOR_ARCHITECTURE_ARM");
      break;
   case PROCESSOR_ARCHITECTURE_IA64:
      V_BSTR(&v) = ::SysAllocString(L"PROCESSOR_ARCHITECTURE_IA64");
      break;
   case PROCESSOR_ARCHITECTURE_ALPHA64:
      V_BSTR(&v) = ::SysAllocString(L"PROCESSOR_ARCHITECTURE_ALPHA64");
      break;
   case PROCESSOR_ARCHITECTURE_UNKNOWN:
      V_BSTR(&v) = ::SysAllocString(L"PROCESSOR_ARCHITECTURE_UNKNOWN");
      break;
   default:
      V_BSTR(&v) = ::SysAllocString(L"Unknown");
   }
   pElement->setAttribute(archName, v);
   // Recycle variant
   SysFreeString(V_BSTR(&v));

   //
   // Set level
   //
   V_VT(&v) = VT_BSTR;
   if (PROCESSOR_ARCHITECTURE_INTEL == si.wProcessorArchitecture)
   {
      switch (si.wProcessorLevel)
      {
      case 3:
         V_BSTR(&v) = ::SysAllocString(L"Intel 30386");
         break;
      case 4:
         V_BSTR(&v) = ::SysAllocString(L"Intel 80486");
         break;
      case 5:
         V_BSTR(&v) = ::SysAllocString(L"Intel Pentium");
         break;
      case 6:
         V_BSTR(&v) = ::SysAllocString(L"Intel Pentium Pro or Pentium II");
         break;
      default:
         V_BSTR(&v) = ::SysAllocString(L"Unknown");
      }
   }
   pElement->setAttribute(levelName, v);
   // Recycle variant
   SysFreeString(V_BSTR(&v));

   //
   // Set num of processors
   //
   V_VT(&v) = VT_I4;
   V_I4(&v) = si.dwNumberOfProcessors;
   pElement->setAttribute(numberName, v);

CleanUp:
   ::SysFreeString(nodeName);
   ::SysFreeString(archName);
   ::SysFreeString(levelName);
   ::SysFreeString(numberName);
   SAFERELEASE(pElement);

   return pNode;
}

//-----------------------------------------------------------------------------
// CExceptionReport::CreateOSNode
//
//
//
MSXML::IXMLDOMNode* 
CExceptionReport::CreateOSNode(MSXML::IXMLDOMDocument* pDoc)
{
   MSXML::IXMLDOMNode*     pNode    = NULL;
   MSXML::IXMLDOMElement*  pElement = NULL;
   BSTR nodeName                    = ::SysAllocString(L"OperatingSystem");
   BSTR majorName                   = ::SysAllocString(L"MajorVersion");
   BSTR minorName                   = ::SysAllocString(L"MinorVersion");
   BSTR buildName                   = ::SysAllocString(L"BuildNumber");
   BSTR csdName                     = ::SysAllocString(L"CSDVersion");
   OSVERSIONINFO oi;
   VARIANT v;

   // Create exception record node
   pNode = CreateDOMNode(pDoc, MSXML::NODE_ELEMENT, nodeName);

   // Get element interface
   CHECKHR(pNode->QueryInterface(MSXML::IID_IXMLDOMElement, (void**)&pElement));

   //
   // Get OS info
   //
   oi.dwOSVersionInfoSize = sizeof(oi);
   GetVersionEx(&oi);

   //
   // Set major version
   //
   V_VT(&v) = VT_I4;
   V_I4(&v) = oi.dwMajorVersion;
   pElement->setAttribute(majorName, v);

   //
   // Set minor version
   //
   V_VT(&v) = VT_I4;
   V_I4(&v) = oi.dwMinorVersion;
   pElement->setAttribute(minorName, v);

   //
   // Set build version
   //
   V_VT(&v) = VT_I4;
   V_I4(&v) = oi.dwBuildNumber;
   pElement->setAttribute(buildName, v);

   //
   // Set CSD version
   //
   V_VT(&v) = VT_BSTR;
   V_BSTR(&v) = A2BSTR(oi.szCSDVersion);
   pElement->setAttribute(csdName, v);
   ::SysFreeString(V_BSTR(&v));

CleanUp:
   ::SysFreeString(nodeName);
   ::SysFreeString(majorName);
   ::SysFreeString(minorName);
   ::SysFreeString(buildName);
   ::SysFreeString(csdName);
   SAFERELEASE(pElement);

   return pNode;
}

//-----------------------------------------------------------------------------
// CExceptionReport::CreateModulesNode
//
//
//
MSXML::IXMLDOMNode* 
CExceptionReport::CreateModulesNode(MSXML::IXMLDOMDocument* pDoc)
{
   MSXML::IXMLDOMNode*     pNode    = NULL;
   MSXML::IXMLDOMNode*     pNode2   = NULL;
   MSXML::IXMLDOMNode*     pNewNode = NULL;
   MSXML::IXMLDOMElement*  pElement = NULL;
   MSXML::IXMLDOMElement*  pElement2= NULL;
   BSTR nodeName                    = ::SysAllocString(L"Modules");
   BSTR nodeName2                   = ::SysAllocString(L"Module");
   BSTR fullPath                    = ::SysAllocString(L"FullPath");
   BSTR baseAddrName                = ::SysAllocString(L"BaseAddress");
   BSTR sizeName                    = ::SysAllocString(L"Size");
   BSTR timeStampName               = ::SysAllocString(L"TimeStamp");
   BSTR fileVerName                 = ::SysAllocString(L"FileVersion");
   BSTR prodVerName                 = ::SysAllocString(L"ProductVersion");

   CString sAddr;
   VARIANT v;


   // Create modules node
   pNode = CreateDOMNode(pDoc, MSXML::NODE_ELEMENT, nodeName);

   //
   // Add module information (freeing storage as we go)
   // 
   MINIDUMP_MODULE_CALLBACK item;
   std::vector<MINIDUMP_MODULE_CALLBACK>::iterator iter;
   for (iter = m_modules.begin(); iter != m_modules.end(); iter++)
   {
	   item = *iter;
      // Create module node
      pNode2 = CreateDOMNode(pDoc, MSXML::NODE_ELEMENT, nodeName2);

      // Get element interface
      CHECKHR(pNode2->QueryInterface(MSXML::IID_IXMLDOMElement, (void**)&pElement));

      //
      // Set full path
      //
      V_VT(&v) = VT_BSTR;
      V_BSTR(&v) = W2BSTR(item.FullPath);
      pElement->setAttribute(fullPath, v);
      // Recycle variant
      SysFreeString(V_BSTR(&v));

      //
      // Set base address
      //
      sAddr.Format(_T("0x%08x"), item.BaseOfImage);
      V_VT(&v) = VT_BSTR;
      V_BSTR(&v) = sAddr.AllocSysString();
      pElement->setAttribute(baseAddrName, v);
      // Recycle variant
      SysFreeString(V_BSTR(&v));

      //
      // Set module size
      //
      sAddr.Format(_T("0x%08x"), item.SizeOfImage);
      V_VT(&v) = VT_BSTR;
      V_BSTR(&v) = sAddr.AllocSysString();
      pElement->setAttribute(sizeName, v);
      // Recycle variant
      SysFreeString(V_BSTR(&v));

      //
      // Set timestamp
      //
      FILETIME    ft = CUtility::getLastWriteFileTime(item.FullPath);
      SYSTEMTIME  st = {0};

      FileTimeToSystemTime(&ft, &st);

      sAddr.Format(
         _T("%02u/%02u/%04u %02u:%02u:%02u"), 
         st.wMonth, 
         st.wDay, 
         st.wYear, 
         st.wHour, 
         st.wMinute, 
         st.wSecond);

      V_VT(&v) = VT_BSTR;
      V_BSTR(&v) = sAddr.AllocSysString();
      pElement->setAttribute(timeStampName, v);
      // Recycle variant
      SysFreeString(V_BSTR(&v));

      //
      // Set file version
      //
      sAddr.Format("%d.%d.%d.%d", 
                   HIWORD(item.VersionInfo.dwFileVersionMS),
                   LOWORD(item.VersionInfo.dwFileVersionMS),
                   HIWORD(item.VersionInfo.dwFileVersionLS),
                   LOWORD(item.VersionInfo.dwFileVersionLS));
      V_VT(&v) = VT_BSTR;
      V_BSTR(&v) = sAddr.AllocSysString();
      pElement->setAttribute(fileVerName, v);
      // Recycle variant
      SysFreeString(V_BSTR(&v));

      //
      // Set product version
      //
      sAddr.Format("%d.%d.%d.%d", 
                   HIWORD(item.VersionInfo.dwProductVersionMS),
                   LOWORD(item.VersionInfo.dwProductVersionMS),
                   HIWORD(item.VersionInfo.dwProductVersionLS),
                   LOWORD(item.VersionInfo.dwProductVersionLS));
      V_VT(&v) = VT_BSTR;
      V_BSTR(&v) = sAddr.AllocSysString();
      pElement->setAttribute(prodVerName, v);
      // Recycle variant
      SysFreeString(V_BSTR(&v));

      //
      // Append module to modules
      //
      pNode->appendChild(pNode2, &pNewNode);
      // The XML Document should now own the node.
      SAFERELEASE(pNode2);
      SAFERELEASE(pElement2);
      SAFERELEASE(pNewNode);

	  free(item.FullPath);
   }
   m_modules.clear();

CleanUp:

   ::SysFreeString(nodeName);
   ::SysFreeString(nodeName2);
   ::SysFreeString(fullPath);
   ::SysFreeString(baseAddrName);
   ::SysFreeString(sizeName);
   ::SysFreeString(timeStampName);
   ::SysFreeString(fileVerName);
   ::SysFreeString(prodVerName);
   SAFERELEASE(pNode2);
   SAFERELEASE(pNewNode);
   SAFERELEASE(pElement);
   SAFERELEASE(pElement2);

   return pNode;
}

//-----------------------------------------------------------------------------
// CreateMsgNode
//
// Builds the application-defined message node
//
MSXML::IXMLDOMNode * 
CExceptionReport::CreateMsgNode(MSXML::IXMLDOMDocument* pDoc, BSTR message)
{
   MSXML::IXMLDOMNode*     pNode    = NULL;
   MSXML::IXMLDOMElement*  pElement = NULL;
   BSTR nodeName                    = ::SysAllocString(L"ApplicationDescription");

   // Create CrashDescription record node
   pNode = CreateDOMNode(pDoc, MSXML::NODE_ELEMENT, nodeName);

   // Get element interface
   CHECKHR(pNode->QueryInterface(MSXML::IID_IXMLDOMElement, (void**)&pElement));

   pElement->put_text(message);

CleanUp:
   ::SysFreeString(nodeName);
   SAFERELEASE(pElement);

   return pNode;
}

//-----------------------------------------------------------------------------
// CreateWalkbackEntryNode
//
// Create a single node in the stack walback
//
void
CExceptionReport::CreateWalkbackEntryNode(DWORD address, const char *ImageName,
									  const char *FunctionName, DWORD functionDisp,
									  const char *Filename, DWORD LineNumber, DWORD lineDisp,
									  void *data)
{
   MSXML::IXMLDOMNode*     pNode    = NULL;
   MSXML::IXMLDOMElement*  pElement = NULL;
   MSXML::IXMLDOMNode*     pNewNode = NULL;
   CString sAddr;
   BSTR nodeName                    = ::SysAllocString(L"Frame");
   BSTR frameName					= ::SysAllocString(L"FrameNumber");
   BSTR addrName					= ::SysAllocString(L"ReturnAddress");
   BSTR moduleName					= ::SysAllocString(L"ModuleName");
   BSTR funcName					= ::SysAllocString(L"FunctionName");
   BSTR funcDispName				= ::SysAllocString(L"FunctionDisplacement");
   BSTR fileName					= ::SysAllocString(L"Filename");
   BSTR lineName					= ::SysAllocString(L"LineNumber");
   BSTR lineDispName				= ::SysAllocString(L"LineDisplacement");
   CExceptionReport	*self = reinterpret_cast<CExceptionReport *>(data);

   // Create frame record node
   pNode = self->CreateDOMNode(self->m_stack_doc, MSXML::NODE_ELEMENT, nodeName);

   // Get element interface
   CHECKHR(pNode->QueryInterface(MSXML::IID_IXMLDOMElement, (void**)&pElement));


   VARIANT v;

   self->m_frameNumber++;
   sAddr.Format(_T("%d"), self->m_frameNumber);
   V_VT(&v) = VT_BSTR;
   V_BSTR(&v) = sAddr.AllocSysString();
   pElement->setAttribute(frameName, v);
   // Recycle variant
   SysFreeString(V_BSTR(&v));

   sAddr.Format(_T("0x%x"), address);
   V_VT(&v) = VT_BSTR;
   V_BSTR(&v) = sAddr.AllocSysString();
   pElement->setAttribute(addrName, v);
   // Recycle variant
   SysFreeString(V_BSTR(&v));

   if (ImageName != NULL) {
		V_VT(&v) = VT_BSTR;
		V_BSTR(&v) = CString(ImageName).AllocSysString();
		pElement->setAttribute(moduleName, v);
		// Recycle variant
		SysFreeString(V_BSTR(&v));
   }

   if (FunctionName != NULL) {
		V_VT(&v) = VT_BSTR;
		V_BSTR(&v) = CString(FunctionName).AllocSysString();
		pElement->setAttribute(funcName, v);
		// Recycle variant
		SysFreeString(V_BSTR(&v));
		sAddr.Format(_T("0x%x"), functionDisp);
		V_VT(&v) = VT_BSTR;
		V_BSTR(&v) = sAddr.AllocSysString();
		pElement->setAttribute(funcDispName, v);
		// Recycle variant
		SysFreeString(V_BSTR(&v));
   }

   if (Filename != NULL) {
		V_VT(&v) = VT_BSTR;
		V_BSTR(&v) = CString(Filename).AllocSysString();
		pElement->setAttribute(fileName, v);
		// Recycle variant
		SysFreeString(V_BSTR(&v));

		sAddr.Format(_T("%d"), LineNumber);
		V_VT(&v) = VT_BSTR;
		V_BSTR(&v) = sAddr.AllocSysString();
		pElement->setAttribute(lineName, v);
		// Recycle variant
		SysFreeString(V_BSTR(&v));

		sAddr.Format(_T("0x%x"), lineDisp);
		V_VT(&v) = VT_BSTR;
		V_BSTR(&v) = sAddr.AllocSysString();
		pElement->setAttribute(lineDispName, v);
		// Recycle variant
		SysFreeString(V_BSTR(&v));
   }
   // add to walkback element

   self->m_stack_element->appendChild(pNode, &pNewNode);
   SAFERELEASE(pNewNode);
   // The XML Document should now own the node.
CleanUp:
   SAFERELEASE(pNode);
   SAFERELEASE(pElement);
   ::SysFreeString(nodeName);
   ::SysFreeString(frameName);
   ::SysFreeString(addrName);
   ::SysFreeString(moduleName);
   ::SysFreeString(funcName);
   ::SysFreeString(funcDispName);
   ::SysFreeString(fileName);
   ::SysFreeString(lineName);
   ::SysFreeString(lineDispName);
}

//-----------------------------------------------------------------------------
// CreateWalkbackNode
//
// Builds the stack walkback list
//
MSXML::IXMLDOMNode * 
CExceptionReport::CreateWalkbackNode(MSXML::IXMLDOMDocument* pDoc, CONTEXT *pContext)
{
   MSXML::IXMLDOMNode*     pNode    = NULL;

   MSXML::IXMLDOMElement*  pElement = NULL;
   BSTR nodeName                    = ::SysAllocString(L"CallStack");

   // Create CallStack record node
   pNode = CreateDOMNode(pDoc, MSXML::NODE_ELEMENT, nodeName);

   // Get element interface
   CHECKHR(pNode->QueryInterface(MSXML::IID_IXMLDOMElement, (void**)&pElement));

   // create the trace
   //  set static variables for use by CreateWalkbackEntryNode
   m_stack_element = pElement;
   m_stack_doc = pDoc;
   m_frameNumber = 0;
   // If no context is supplied, skip 1 frames:
   //  1 this function
   //  ??
   DoStackTrace(pContext == NULL ? 1 : 0, 9999, CreateWalkbackEntryNode, pContext, this);

CleanUp:
   ::SysFreeString(nodeName);
   SAFERELEASE(pElement);

   return pNode;
}

//-----------------------------------------------------------------------------
// CExceptionReport::miniDumpCallback
//
// Mini dump module callback.  Hit once for each module processed by
// MiniDumpWriteDump.  Builds a linked list of all module names which is
// eventually used to create the <modules> node in the XML log file.
//
BOOL CALLBACK 
CExceptionReport::miniDumpCallback(PVOID data,
                                   CONST PMINIDUMP_CALLBACK_INPUT CallbackInput,
                                   PMINIDUMP_CALLBACK_OUTPUT)
{
   CExceptionReport *self = reinterpret_cast<CExceptionReport*>(data);
   if (ModuleCallback == CallbackInput->CallbackType)
   {
      MINIDUMP_MODULE_CALLBACK item;

      item = CallbackInput->Module;
      item.FullPath = _wcsdup(CallbackInput->Module.FullPath);
	  self->m_modules.push_back(item);
   }

   return TRUE;
}