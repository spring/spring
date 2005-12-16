// AutoUpdater.h: interface for the CAutoUpdater class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_AUTOUPDATER_H__227B2B21_B6AE_4164_B3A5_BFDAAF13D85D__INCLUDED_)
#define AFX_AUTOUPDATER_H__227B2B21_B6AE_4164_B3A5_BFDAAF13D85D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <Wininet.h>

#define LOCATION_UPDATE_FILE_CHECK _T("NTAI.upd")

class CAutoUpdater  
{
public:
	CAutoUpdater(string version);
	virtual ~CAutoUpdater();

	enum ErrorType { Success, InternetConnectFailure, InternetSessionFailure, 
						ConfigDownloadFailure, FileDownloadFailure, NoExecutableVersion,
						UpdateNotRequired, UpdateNotComplete };

	ErrorType CheckForUpdate(LPCTSTR UpdateServerURL);	
	HINTERNET GetSession(CString &URL);

	bool InternetOkay();
	bool DownloadConfig(HINTERNET hSession, BYTE *pBuf, DWORD bufSize);
	bool DownloadFile(HINTERNET hSession, LPCTSTR localFile);

	CString GetFileVersion(LPCTSTR file);
	int		CompareVersions(CString ver1, CString ver2);
	bool	IsDigits(CString text);
	CString GetExecutable();
	bool	Switch(CString executable, CString update, bool WaitForReboot);

private:
	HINTERNET hInternet;
	string ver;
};

#endif // !defined(AFX_AUTOUPDATER_H__227B2B21_B6AE_4164_B3A5_BFDAAF13D85D__INCLUDED_)
