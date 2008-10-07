// SelectionEditorDlg.h : header file
//

#pragma once
#include "afxwin.h"


// CSelectionEditorDlg dialog
class CSelectionEditorDlg : public CDialog
{
// Construction
public:
	CSelectionEditorDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_SELECTIONEDITOR_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	CListBox keys;
	CButton editButton;
	CButton newButton;
	afx_msg void OnDelButton();
	afx_msg void OnNewButton();
	afx_msg void OnEditButton();
	CEdit value;
	afx_msg void OnLbnSelchangeList2();
};
