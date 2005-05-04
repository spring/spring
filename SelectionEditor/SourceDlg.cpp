// SourceDlg.cpp : implementation file
//

#include "stdafx.h"
#include "SelectionEditor.h"
#include "SourceDlg.h"
#include ".\sourcedlg.h"
#include "datastore.h"
#include "filterdlg.h"
using namespace std;

// CSourceDlg dialog

IMPLEMENT_DYNAMIC(CSourceDlg, CDialog)
CSourceDlg::CSourceDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSourceDlg::IDD, pParent)
	, tset(0)
{
}

CSourceDlg::~CSourceDlg()
{
}

void CSourceDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDCANCEL, allmap);
	DDX_Control(pDX, IDC_EDIT1, mousedist);
}

BOOL CSourceDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	
	((CButton*)GetDlgItem(IDC_RADIO1))->SetCheck(true);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

BEGIN_MESSAGE_MAP(CSourceDlg, CDialog)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
END_MESSAGE_MAP()


// CSourceDlg message handlers

void CSourceDlg::OnBnClickedOk()
{
	if(((CButton*)GetDlgItem(IDC_RADIO1))->GetCheck())
		datastore.curString+="AllMap+";
	if(((CButton*)GetDlgItem(IDC_RADIO2))->GetCheck())
		datastore.curString+="Visible+";
	if(((CButton*)GetDlgItem(IDC_RADIO3))->GetCheck()){
		datastore.curString+="FromMouse_";
		char c[100];
		mousedist.GetLine(0,c);
		datastore.curString+=c;
		datastore.curString+="+";
	}
	if(((CButton*)GetDlgItem(IDC_RADIO4))->GetCheck())
		datastore.curString+="PrevSelection+";

	CFilterDlg dlg;
	INT_PTR nResponse = dlg.DoModal();

	OnOK();
}

void CSourceDlg::OnBnClickedCancel()
{
	datastore.curKey="";
	datastore.curString="";
	OnCancel();
}
