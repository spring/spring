#pragma once
#include "afxwin.h"


// CHotKeyDlg dialog

class CHotKeyDlg : public CDialog
{
	DECLARE_DYNAMIC(CHotKeyDlg)

public:
	CHotKeyDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CHotKeyDlg();

// Dialog Data
	enum { IDD = IDD_DIALOG1 };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CEdit hotkey;
	afx_msg void OnBnClickedOk();
	CButton shiftKey;
	CButton ctrlKey;
	CButton altKey;
};
