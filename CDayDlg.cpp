// CDayDlg.cpp : 구현 파일
#include "pch.h"
#include "Mafia43.h"
#include "CDayDlg.h"
#include "afxdialogex.h"
#include "SharedStructures.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNAMIC(CDayDlg, CDialogEx)

CDayDlg::CDayDlg(CWnd* pParent, CClientSocket* pSocket,
	CString strMyUID, CString strMyNickname, CString strMyRole,
	const std::vector<RoomPlayerInfo>& players)
	: CDialogEx(IDD_DAY, pParent)
	, m_pSocket(pSocket)
	, m_strMyUID(strMyUID)
	, m_strMyNickname(strMyNickname)
	, m_strMyRole(strMyRole)
	, m_vecDayPlayers(players)
	, m_nDayTimeLimit(120) // 낮 120초
{
}

CDayDlg::~CDayDlg()
{
}

void CDayDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_RICH_CHAT, m_richChat);
	DDX_Control(pDX, IDC_LIST_VOTE, m_listVote);
	DDX_Text(pDX, IDC_EDIT_CHAT, m_strChatMsg);
}

BEGIN_MESSAGE_MAP(CDayDlg, CDialogEx)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_BUTTON_SEND_CHAT, &CDayDlg::OnBnClickedButtonSendChat)
	ON_BN_CLICKED(IDC_BUTTON_VOTE, &CDayDlg::OnBnClickedButtonVote)
	ON_MESSAGE(WM_USER_RECV_MSG, &CDayDlg::OnReceiveMsg)
END_MESSAGE_MAP()

BOOL CDayDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	if (m_pSocket == nullptr) {
		AfxMessageBox(_T("소켓 오류")); OnCancel(); return FALSE;
	}

	m_listVote.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	m_listVote.InsertColumn(0, _T("UID"), LVCFMT_LEFT, 0);
	m_listVote.InsertColumn(1, _T("플레이어"), LVCFMT_LEFT, 150);
	m_listVote.InsertColumn(2, _T("상태"), LVCFMT_LEFT, 80);

	CString strRoleDisplay;
	// [수정] (LPCTSTR) 캐스팅 추가
	strRoleDisplay.Format(_T("역할: %s"), (LPCTSTR)m_strMyRole);
	SetDlgItemText(IDC_STATIC, strRoleDisplay);

	// 죽었는지 확인
	for (const auto& p : m_vecDayPlayers) {
		if (p.strUID == m_strMyUID && !p.bIsAlive) {
			AfxMessageBox(_T("마피아에게 습격당해 사망했습니다... 로비로 이동합니다."));
			OnCancel();
			return TRUE;
		}
	}

	PopulateVoteList();
	AppendTextToRichEdit(_T("[알림] 낮이 되었습니다. 토론을 시작하세요.\r\n"), RGB(0, 0, 255));

	UpdateTimerDisplay();
	SetTimer(1, 1000, NULL);

	return TRUE;
}

void CDayDlg::PopulateVoteList()
{
	m_listVote.DeleteAllItems();
	int nItem = 0;
	for (const auto& player : m_vecDayPlayers)
	{
		if (player.bIsAlive)
		{
			m_listVote.InsertItem(nItem, player.strUID);
			m_listVote.SetItemText(nItem, 1, player.strName);
			m_listVote.SetItemText(nItem, 2, _T("생존"));
			nItem++;
		}
	}
}

void CDayDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == 1) {
		if (m_nDayTimeLimit > 0) {
			m_nDayTimeLimit--;
			UpdateTimerDisplay();
		}
		else {
			KillTimer(1);
			AppendTextToRichEdit(_T("[알림] 토론 시간이 종료되었습니다.\r\n"), RGB(255, 0, 0));
			GetDlgItem(IDC_BUTTON_VOTE)->EnableWindow(FALSE);
		}
	}
	CDialogEx::OnTimer(nIDEvent);
}

void CDayDlg::UpdateTimerDisplay()
{
	CString strTime;
	strTime.Format(_T("남은 시간: %02d:%02d"), m_nDayTimeLimit / 60, m_nDayTimeLimit % 60);

	// [수정] 리소스 뷰에 있는 ID인 IDC_STATIC_TIME 사용
	SetDlgItemText(IDC_STATIC_TIME, strTime);
}

void CDayDlg::AppendTextToRichEdit(CString strText, COLORREF color)
{
	if (strText.Right(2) != _T("\r\n")) strText += _T("\r\n");
	CHARFORMAT cf; ZeroMemory(&cf, sizeof(CHARFORMAT));
	cf.cbSize = sizeof(CHARFORMAT); cf.dwMask = CFM_COLOR; cf.crTextColor = color;
	long nLen = m_richChat.GetWindowTextLength();
	m_richChat.SetSel(nLen, nLen); m_richChat.SetSelectionCharFormat(cf);
	m_richChat.ReplaceSel(strText); m_richChat.PostMessage(WM_VSCROLL, SB_BOTTOM, 0);
}

void CDayDlg::OnBnClickedButtonSendChat()
{
	UpdateData(TRUE);
	if (m_strChatMsg.IsEmpty() || !m_pSocket) return;

	CStringA strJson;
	// [수정] (LPCSTR) 캐스팅 확인
	strJson.Format("{\"op\":\"DAY_CHAT\", \"text\":\"%s\"}", (LPCSTR)CStr_to_CStrA(m_strChatMsg));
	m_pSocket->SendJson(strJson);

	m_strChatMsg = _T(""); UpdateData(FALSE); GetDlgItem(IDC_EDIT_CHAT)->SetFocus();
}

void CDayDlg::OnBnClickedButtonVote()
{
	int nItem = m_listVote.GetNextItem(-1, LVNI_SELECTED);
	if (nItem == -1) { AfxMessageBox(_T("투표할 대상을 선택하세요.")); return; }

	CString strTargetUID = m_listVote.GetItemText(nItem, 0);
	if (strTargetUID == m_strMyUID) { AfxMessageBox(_T("자신에게 투표할 수 없습니다.")); return; }

	if (m_pSocket) {
		CStringA strJson;
		// [수정] (LPCSTR) 캐스팅 확인
		strJson.Format("{\"op\":\"DAY_VOTE\", \"target\":\"%s\"}", (LPCSTR)CStr_to_CStrA(strTargetUID));
		m_pSocket->SendJson(strJson);
		GetDlgItem(IDC_BUTTON_VOTE)->EnableWindow(FALSE);
		AppendTextToRichEdit(_T("[알림] 투표를 완료했습니다.\r\n"), RGB(0, 0, 255));
	}
}

afx_msg LRESULT CDayDlg::OnReceiveMsg(WPARAM wParam, LPARAM lParam)
{
	CStringA* pJsonA = (CStringA*)wParam;
	if (pJsonA) {
		CStringA strJson = *pJsonA; delete pJsonA;
		ProcessServerMessage(strJson);
	}
	return 0;
}

void CDayDlg::ProcessServerMessage(CStringA strJsonA)
{
	if (strJsonA.Find("\"op\": \"CHAT\"") != -1) ParseChat(strJsonA);
	else if (strJsonA.Find("\"op\": \"DAY_RESULT\"") != -1)
	{
		CString strVictim = _T("");
		int nVic = strJsonA.Find("\"victim\": \"");
		if (nVic != -1) {
			CStringA sVal = strJsonA.Mid(nVic + 11);
			sVal = sVal.Left(sVal.Find('\"'));
			strVictim = CString(CA2T(sVal));
		}

		if (!strVictim.IsEmpty()) {
			if (m_strMyUID == strVictim || m_strMyNickname.Find(strVictim) != -1) {
				KillTimer(1);
				AfxMessageBox(_T("투표로 처형되었습니다... 로비로 이동합니다."));
				OnCancel();
				return;
			}
			CString msg;
			// [수정] (LPCTSTR) 캐스팅 추가
			msg.Format(_T("[속보] %s 님이 처형되었습니다.\r\n"), (LPCTSTR)strVictim);
			AppendTextToRichEdit(msg, RGB(255, 0, 0));
		}
		else {
			AppendTextToRichEdit(_T("[알림] 아무도 처형되지 않았습니다.\r\n"), RGB(0, 100, 0));
		}
	}
	else if (strJsonA.Find("\"op\": \"ROOM_STATE\"") != -1) {
		ParseRoomState(strJsonA);
		for (const auto& p : m_vecDayPlayers) {
			if (p.strUID == m_strMyUID && !p.bIsAlive) {
				KillTimer(1);
				AfxMessageBox(_T("당신은 사망했습니다. 로비로 이동합니다."));
				OnCancel();
				return;
			}
		}
	}
	else if (strJsonA.Find("\"phase\": \"NIGHT\"") != -1) {
		KillTimer(1);
		AppendTextToRichEdit(_T("[알림] 밤이 되었습니다.\r\n"), RGB(255, 0, 0));
		OnOK();
	}
	else if (strJsonA.Find("\"op\": \"GAME_END\"") != -1) {
		KillTimer(1);
		AfxMessageBox(_T("게임이 종료되었습니다!"));
		OnCancel();
	}
	else if (strJsonA.Find("\"op\": \"ERROR\"") != -1) {
		AfxMessageBox(CStrA_to_CStr(strJsonA));
	}
}

void CDayDlg::ParseChat(const CStringA& strJsonA)
{
	int nFrom = strJsonA.Find("\"from\": \"");
	int nText = strJsonA.Find("\"text\": \"");
	if (nFrom != -1 && nText != -1) {
		CStringA sFrom = strJsonA.Mid(nFrom + 9); sFrom = sFrom.Left(sFrom.Find('\"'));
		CStringA sText = strJsonA.Mid(nText + 9); sText = sText.Left(sText.Find('\"'));

		CString msg;
		// [수정] (LPCTSTR) 캐스팅 추가
		msg.Format(_T("%s: %s"), (LPCTSTR)CStrA_to_CStr(sFrom), (LPCTSTR)CStrA_to_CStr(sText));
		AppendTextToRichEdit(msg);
	}
}

void CDayDlg::ParseRoomState(const CStringA& strJsonA) {
	m_vecDayPlayers.clear();
}

void CDayDlg::ParseVoteResult(const CStringA& strJsonA) {}

CStringA CDayDlg::CStr_to_CStrA(const CString& strT) { CT2A utf8(strT, CP_UTF8); return CStringA(utf8); }
CString CDayDlg::CStrA_to_CStr(const CStringA& strA) { CA2T utf8(strA, CP_UTF8); return CString(utf8); }