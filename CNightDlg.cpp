// CNightDlg.cpp: 구현 파일
//

#include "pch.h"
#include "Mafia43.h"
#include "afxdialogex.h"
#include "CNightDlg.h"


// CNightDlg 대화 상자

IMPLEMENT_DYNAMIC(CNightDlg, CDialogEx)

CNightDlg::CNightDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_NIGHT_DIALOG, pParent)
{
	m_pSocket = nullptr;
}

CNightDlg::~CNightDlg()
{
}

void CNightDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CNightDlg, CDialogEx)
END_MESSAGE_MAP()


// CNightDlg 메시지 처리기
