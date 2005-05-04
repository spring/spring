#pragma once
#include "afxwin.h"


// CSourceDlg dialog

class CSourceDlg : public CDialog
{
	DECLARE_DYNAMIC(CSourceDlg)

public:
	CSourceDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSourceDlg();
	BOOL OnInitDialog();

// Dialog Data
	enum { IDD = IDD_DIALOG2 };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CButton allmap;
	CEdit mousedist;
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	int tset;
};
