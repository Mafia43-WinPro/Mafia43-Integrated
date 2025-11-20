// CNightDlg.cpp: 구현 파일
//

#include "pch.h"
#include "Mafia43.h"
#include "afxdialogex.h"
#include "CNightDlg.h"
#include "CClientSocket.h" // m_pSocket을 사용하기 위해 include 추가

// CNightDlg 대화 상자

IMPLEMENT_DYNAMIC(CNightDlg, CDialogEx)

CNightDlg::CNightDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_NIGHT_DIALOG, pParent) // TODO: '밤 화면'의 IDD가 맞는지 확인
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
	// TODO: 밤에 능력 사용/종료 버튼이 생기면 여기에 ON_BN_CLICKED 추가
END_MESSAGE_MAP()


// CNightDlg 메시지 처리기

BOOL CNightDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 부모(CMafia43Dlg)로부터 m_pSocket을 잘 받았는지 확인
	if (m_pSocket == nullptr)
	{
		AfxMessageBox(_T("소켓이 연결되지 않았습니다."));
		CDialogEx::OnCancel(); // OnCancel로 닫으면 게임 루프가 중단됩니다.
		return FALSE;
	}

	// TODO: 밤 화면 타이머 시작 (예: SetTimer(1, 1000, NULL))
	// TODO: 내 역할(m_strMyRole)에 따라 능력 버튼 활성화/비활성화

	return TRUE;
}

/**
 * @brief [임시 추가] OnOK 가상 함수 재정의
 * * 이 다이얼로그가 'IDOK'로 닫힐 때 (예: Enter 키, 또는 IDOK를 반환하는 버튼 클릭 시)
 * 닫히기 직전에 NEXT_PHASE JSON을 전송합니다.
 */
void CNightDlg::OnOK()
{
	// TODO: 
	// 여기에 '밤' 능력 사용(예: 마피아의 킬) JSON을
	// m_pSocket->SendJson(...)으로 전송하는 로직이 필요합니다.

	// [NOT_DAY 오류 해결]
	// 밤 행동이 끝났으므로, 서버에게 "낮으로 변경"을 요청합니다.
	if (m_pSocket)
	{
		m_pSocket->SendJson("{\"op\": \"NEXT_PHASE\"}");
	}

	// CDialogEx::OnOK()를 호출하여 다이얼로그를 정상 종료합니다.
	// (이래야 OnGameStart 루프가 CDayDlg로 넘어갑니다)
	CDialogEx::OnOK();
}