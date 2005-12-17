// FilterDlg.cpp : implementation file
//

#include "stdafx.h"
#include "SelectionEditor.h"
#include "FilterDlg.h"
#include ".\filterdlg.h"
#include "datastore.h"

using namespace std;

// CFilterDlg dialog

IMPLEMENT_DYNAMIC(CFilterDlg, CDialog)
CFilterDlg::CFilterDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CFilterDlg::IDD, pParent)
{
}

CFilterDlg::~CFilterDlg()
{
}

void CFilterDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BOOL CFilterDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	
	((CButton*)GetDlgItem(IDC_RADIO5))->SetCheck(true);
	((CButton*)GetDlgItem(IDC_CHECK23))->SetCheck(true);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

BEGIN_MESSAGE_MAP(CFilterDlg, CDialog)
	ON_EN_CHANGE(IDC_EDIT2, OnEnChangeEdit2)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
END_MESSAGE_MAP()


// CFilterDlg message handlers

void CFilterDlg::OnEnChangeEdit2()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
}

void CFilterDlg::OnBnClickedOk()
{
	char buf[100];
	if(((CButton*)GetDlgItem(IDC_CHECK1))->GetCheck())
		datastore.curString+="_Builder";
	if(((CButton*)GetDlgItem(IDC_CHECK2))->GetCheck())
		datastore.curString+="_Not_Builder";

	if(((CButton*)GetDlgItem(IDC_CHECK3))->GetCheck())
		datastore.curString+="_Building";
	if(((CButton*)GetDlgItem(IDC_CHECK4))->GetCheck())
		datastore.curString+="_Not_Building";

	if(((CButton*)GetDlgItem(IDC_CHECK5))->GetCheck())
		datastore.curString+="_Commander";
	if(((CButton*)GetDlgItem(IDC_CHECK6))->GetCheck())
		datastore.curString+="_Not_Commander";

	if(((CButton*)GetDlgItem(IDC_CHECK7))->GetCheck())
		datastore.curString+="_Transport";
	if(((CButton*)GetDlgItem(IDC_CHECK8))->GetCheck())
		datastore.curString+="_Not_Transport";

	if(((CButton*)GetDlgItem(IDC_CHECK9))->GetCheck())
		datastore.curString+="_Aircraft";
	if(((CButton*)GetDlgItem(IDC_CHECK10))->GetCheck())
		datastore.curString+="_Not_Aircraft";

	if(((CButton*)GetDlgItem(IDC_CHECK11))->GetCheck())
		datastore.curString+="_Weapons";
	if(((CButton*)GetDlgItem(IDC_CHECK12))->GetCheck())
		datastore.curString+="_Not_Weapons";

	if(((CButton*)GetDlgItem(IDC_CHECK14))->GetCheck())
		datastore.curString+="_Not";
	if(((CButton*)GetDlgItem(IDC_CHECK13))->GetCheck() || ((CButton*)GetDlgItem(IDC_CHECK14))->GetCheck()){
		datastore.curString+="_WeaponRange_";
		((CEdit*)GetDlgItem(IDC_EDIT2))->GetLine(0,buf);
		datastore.curString+=buf;
	}
	if(((CButton*)GetDlgItem(IDC_CHECK16))->GetCheck())
		datastore.curString+="_Not";
	if(((CButton*)GetDlgItem(IDC_CHECK15))->GetCheck() || ((CButton*)GetDlgItem(IDC_CHECK16))->GetCheck()){
		datastore.curString+="_AbsoluteHealth_";
		((CEdit*)GetDlgItem(IDC_EDIT3))->GetLine(0,buf);
		datastore.curString+=buf;
	}
	if(((CButton*)GetDlgItem(IDC_CHECK18))->GetCheck())
		datastore.curString+="_Not";
	if(((CButton*)GetDlgItem(IDC_CHECK17))->GetCheck() || ((CButton*)GetDlgItem(IDC_CHECK18))->GetCheck()){
		datastore.curString+="_RelativeHealth_";
		((CEdit*)GetDlgItem(IDC_EDIT4))->GetLine(0,buf);
		datastore.curString+=buf;
	}
	if(((CButton*)GetDlgItem(IDC_CHECK19))->GetCheck())
		datastore.curString+="_InPrevSel";
	if(((CButton*)GetDlgItem(IDC_CHECK20))->GetCheck())
		datastore.curString+="_Not_InPrevSel";

	if(((CButton*)GetDlgItem(IDC_CHECK22))->GetCheck())
		datastore.curString+="_Not";
	if(((CButton*)GetDlgItem(IDC_CHECK21))->GetCheck() || ((CButton*)GetDlgItem(IDC_CHECK22))->GetCheck()){
		datastore.curString+="_NameContain_";
		((CEdit*)GetDlgItem(IDC_EDIT5))->GetLine(0,buf);
		datastore.curString+=buf;
	}
	if(((CButton*)GetDlgItem(IDC_CHECK24))->GetCheck())
		datastore.curString+="_Idle";
	if(((CButton*)GetDlgItem(IDC_CHECK25))->GetCheck())
		datastore.curString+="_Not_Idle";

	if(((CButton*)GetDlgItem(IDC_CHECK26))->GetCheck())
		datastore.curString+="_Radar";
	if(((CButton*)GetDlgItem(IDC_CHECK27))->GetCheck())
		datastore.curString+="_Not_Radar";

	if(((CButton*)GetDlgItem(IDC_CHECK29))->GetCheck())
		datastore.curString+="_Not";
	if(((CButton*)GetDlgItem(IDC_CHECK28))->GetCheck() || ((CButton*)GetDlgItem(IDC_CHECK29))->GetCheck()){
		datastore.curString+="_Category_";
		((CEdit*)GetDlgItem(IDC_EDIT7))->GetLine(0,buf);
		datastore.curString+=buf;
	}
	datastore.curString+="+";

	if(((CButton*)GetDlgItem(IDC_CHECK23))->GetCheck()){
		datastore.curString+="_ClearSelection";
	}

	if(((CButton*)GetDlgItem(IDC_RADIO5))->GetCheck()){
		datastore.curString+="_SelectAll";
	} else if(((CButton*)GetDlgItem(IDC_RADIO6))->GetCheck()){
		datastore.curString+="_SelectOne";
	} else if(((CButton*)GetDlgItem(IDC_RADIO7))->GetCheck()){
		datastore.curString+="_SelectNum_";
		((CEdit*)GetDlgItem(IDC_EDIT6))->GetLine(0,buf);
		datastore.curString+=buf;
	} else if(((CButton*)GetDlgItem(IDC_RADIO8))->GetCheck()){
		datastore.curString+="_SelectPart_";
		((CEdit*)GetDlgItem(IDC_EDIT8))->GetLine(0,buf);
		datastore.curString+=buf;
	}
	datastore.curString+="+";

	OnOK();
}

void CFilterDlg::OnBnClickedCancel()
{
	datastore.curKey="";
	datastore.curString="";
	OnCancel();
}
