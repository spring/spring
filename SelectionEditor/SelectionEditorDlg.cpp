// SelectionEditorDlg.cpp : implementation file
//

#include "stdafx.h"
#include "SelectionEditor.h"
#include "SelectionEditorDlg.h"
#include ".\selectioneditordlg.h"
#include <string>
#include "datastore.h"
#include "hotkeydlg.h"
#include "sourcedlg.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CSelectionEditorDlg dialog

using namespace std;

CSelectionEditorDlg::CSelectionEditorDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSelectionEditorDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CSelectionEditorDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST2, keys);
	DDX_Control(pDX, IDC_BUTTON1, editButton);
	DDX_Control(pDX, IDC_BUTTON2, newButton);
	DDX_Control(pDX, IDC_EDIT1, value);
}

BEGIN_MESSAGE_MAP(CSelectionEditorDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BUTTON3, OnDelButton)
	ON_BN_CLICKED(IDC_BUTTON2, OnNewButton)
	ON_BN_CLICKED(IDC_BUTTON1, OnEditButton)
	ON_LBN_SELCHANGE(IDC_LIST2, OnLbnSelchangeList2)
END_MESSAGE_MAP()


// CSelectionEditorDlg message handlers

BOOL CSelectionEditorDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	for(map<string,string>::iterator ki=datastore.hotkeys.begin();ki!=datastore.hotkeys.end();++ki){
		keys.AddString(ki->first.c_str());
	}
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CSelectionEditorDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CSelectionEditorDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CSelectionEditorDlg::OnDelButton()
{
	CString s;
	keys.GetText(keys.GetCurSel(),s);
	std::string s2(s.GetBuffer());
	datastore.hotkeys.erase(s2);
	keys.DeleteString(keys.GetCurSel());
}

void CSelectionEditorDlg::OnNewButton()
{
	datastore.curKey="";
	datastore.curString="";

	CHotKeyDlg dlg;
	INT_PTR nResponse = dlg.DoModal();

	if(datastore.curKey!="" && datastore.curString!=""){
		keys.AddString(datastore.curKey.c_str());
		datastore.hotkeys[datastore.curKey]=datastore.curString;
	}
}

void CSelectionEditorDlg::OnEditButton()
{
	CString s;
	keys.GetText(keys.GetCurSel(),s);

	datastore.curKey=s.GetBuffer();
	datastore.curString="";

	CSourceDlg dlg;
	INT_PTR nResponse = dlg.DoModal();

	if(datastore.curKey!="" && datastore.curString!=""){
		keys.DeleteString(keys.GetCurSel());
		keys.AddString(datastore.curKey.c_str());
		datastore.hotkeys[datastore.curKey]=datastore.curString;
	}
}

void CSelectionEditorDlg::OnLbnSelchangeList2()
{
	CString s;
	keys.GetText(keys.GetCurSel(),s);

	string s2=datastore.hotkeys[s.GetBuffer()];
	value.SetWindowText(s2.c_str());
}
