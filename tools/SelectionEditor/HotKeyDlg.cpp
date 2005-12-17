// HotKeyDlg.cpp : implementation file
//

#include "stdafx.h"
#include "SelectionEditor.h"
#include "HotKeyDlg.h"
#include ".\hotkeydlg.h"
#include "datastore.h"
#include "sourcedlg.h"
using namespace std;

// CHotKeyDlg dialog

IMPLEMENT_DYNAMIC(CHotKeyDlg, CDialog)
CHotKeyDlg::CHotKeyDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CHotKeyDlg::IDD, pParent)
{
//	hotkey.SetLimitText(1);
}

CHotKeyDlg::~CHotKeyDlg()
{
}

void CHotKeyDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT1, hotkey);
	DDX_Control(pDX, IDC_CHECK1, shiftKey);
	DDX_Control(pDX, IDC_CHECK2, ctrlKey);
	DDX_Control(pDX, IDC_CHECK3, altKey);
}


BEGIN_MESSAGE_MAP(CHotKeyDlg, CDialog)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
END_MESSAGE_MAP()


// CHotKeyDlg message handlers

void CHotKeyDlg::OnBnClickedOk()
{
	UpdateData(0);
	string s;
	if(shiftKey.GetCheck())
		s+="Shift_";
	if(ctrlKey.GetCheck())
		s+="Control_";
	if(altKey.GetCheck())
		s+="Alt_";

	char buf[10];
	hotkey.GetLine(0,buf,10);
	s+=buf;

	datastore.curKey=s;

	CSourceDlg dlg;
	INT_PTR nResponse = dlg.DoModal();

	OnOK();
}
