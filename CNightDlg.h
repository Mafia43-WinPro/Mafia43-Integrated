#pragma once
#include "afxdialogex.h"
#include "CClientSocket.h"

// CNightDlg 대화 상자

class CNightDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CNightDlg)

public:
	CNightDlg(CWnd* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~CNightDlg();
	CClientSocket* m_pSocket;
	// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_NIGHT_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()

public:
	// ▼▼▼ 여기에 님이 전달할 데이터를 받을 변수를 추가합니다 ▼▼▼
	CString m_strMyNickname; // 님의 닉네임
	CString m_strMyRole;     // 님의 직업 (예: "마피아", "경찰")
};