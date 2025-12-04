// CDayDlg.cpp : 구현 파일
#include "pch.h"
#include "Mafia43.h"
#include "Mafia43Dlg.h"
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
	, m_nDayTimeLimit(120) // 낮 시간 120초
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
	strRoleDisplay.Format(_T("역할: %s"), (LPCTSTR)m_strMyRole);
	SetDlgItemText(IDC_STATIC, strRoleDisplay);

	// 1. 방금 밤에 죽었는지 확인 (입장 컷)
	for (const auto& p : m_vecDayPlayers) {
		if (p.strUID == m_strMyUID && !p.bIsAlive) {
			AfxMessageBox(_T("마피아에게 습격당해 사망했습니다... 로비로 이동합니다."));
			OnCancel();
			return TRUE;
		}
	}

	// 2. 초기 리스트 그리기
	PopulateVoteList();
	AppendTextToRichEdit(_T("[알림] 낮이 되었습니다. 토론을 시작하세요.\r\n"), RGB(0, 0, 255));

	// ★★★ [핵심] 서버에게 "최신 명단 줘!" 라고 요청 ★★★
	// 이걸 해야 밤 사이에 죽은 사람이 반영된 리스트를 다시 받습니다.
	m_pSocket->SendJson("{\"op\":\"ROOM_STATE\"}");

	// 3. 타이머 시작
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
		// 모든 플레이어 표시 (죽은 플레이어도 포함)
		CString strDisplayName;
		strDisplayName.Format(_T("Player%d"), player.nPlayerNumber);

		m_listVote.InsertItem(nItem, player.strUID);
		m_listVote.SetItemText(nItem, 1, strDisplayName);

		// 상태를 올바르게 표시
		m_listVote.SetItemText(nItem, 2, player.bIsAlive ? _T("생존") : _T("사망"));

		nItem++;
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
			AppendTextToRichEdit(_T("[알림] 토론 시간이 종료되었습니다. 투표 집계 중...\r\n"), RGB(255, 0, 0));
			GetDlgItem(IDC_BUTTON_VOTE)->EnableWindow(FALSE);

			// 시간이 끝나면 방장이 서버에 신호를 보냄
			bool bAmIHost = false;
			for (const auto& p : m_vecDayPlayers) {
				if (p.strUID == m_strMyUID && p.bIsHost) {
					bAmIHost = true;
					break;
				}
			}

			if (bAmIHost && m_pSocket) {
				m_pSocket->SendJson("{\"op\": \"NEXT_PHASE\"}");
			}
		}
	}
	CDialogEx::OnTimer(nIDEvent);
}

void CDayDlg::UpdateTimerDisplay()
{
	CString strTime;
	strTime.Format(_T("남은 시간: %02d:%02d"), m_nDayTimeLimit / 60, m_nDayTimeLimit % 60);
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
	strJson.Format("{\"op\":\"DAY_CHAT\", \"text\":\"%s\"}", (LPCSTR)CStr_to_CStrA(m_strChatMsg));
	m_pSocket->SendJson(strJson);

	m_strChatMsg = _T(""); UpdateData(FALSE); GetDlgItem(IDC_EDIT_CHAT)->SetFocus();
}

void CDayDlg::OnBnClickedButtonVote()
{
	int nItem = m_listVote.GetNextItem(-1, LVNI_SELECTED);
	if (nItem == -1) { AfxMessageBox(_T("투표할 대상을 선택하세요.")); return; }

	CString strTargetUID = m_listVote.GetItemText(nItem, 0);
	CString strTargetStatus = m_listVote.GetItemText(nItem, 2);

	if (strTargetUID == m_strMyUID) { AfxMessageBox(_T("자신에게 투표할 수 없습니다.")); return; }

	// 죽은 플레이어에게 투표 방지
	if (strTargetStatus == _T("사망")) {
		AfxMessageBox(_T("사망한 플레이어에게 투표할 수 없습니다."));
		return;
	}

	if (m_pSocket) {
		CStringA strJson;
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
		int nVictimNumber = 0;

		// victim UID 파싱
		int nVic = strJsonA.Find("\"victim\": \"");
		if (nVic != -1) {
			CStringA sVal = strJsonA.Mid(nVic + 11);
			sVal = sVal.Left(sVal.Find('\"'));
			strVictim = CString(CA2T(sVal));
		}

		// victim_number 파싱
		int nVicNum = strJsonA.Find("\"victim_number\"");
		if (nVicNum != -1) {
			int c = strJsonA.Find(':', nVicNum);
			CStringA numStr = strJsonA.Mid(c + 1);
			numStr.Trim();
			int endPos = numStr.FindOneOf(",}");
			if (endPos != -1) {
				numStr = numStr.Left(endPos);
				numStr.Trim();
				nVictimNumber = atoi(numStr);
			}
		}

		if (!strVictim.IsEmpty()) {
			if (m_strMyUID == strVictim || m_strMyNickname.Find(strVictim) != -1) {
				KillTimer(1);
				AfxMessageBox(_T("투표로 처형되었습니다... 로비로 이동합니다."));
				OnCancel();
				return;
			}
			CString msg;
			// Player{number} 형식으로 표시
			msg.Format(_T("[속보] Player%d 님이 처형되었습니다.\r\n"), nVictimNumber);
			AppendTextToRichEdit(msg, RGB(255, 0, 0));
		}
		else {
			AppendTextToRichEdit(_T("[알림] 아무도 처형되지 않았습니다.\r\n"), RGB(0, 100, 0));
		}
	}
	// ★ [핵심] 서버가 보내준 최신 명단(ROOM_STATE) 처리
	else if (strJsonA.Find("\"op\": \"ROOM_STATE\"") != -1) {
		ParseRoomState(strJsonA);
		// (여기서 리스트가 갱신되어 죽은 사람은 화면에서 사라짐)

		// 혹시 내가 죽은 걸로 바뀌었는지 한 번 더 확인
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
	// from_number 필드 파싱
	int nFromNumber = 0;
	int nFromNumPos = strJsonA.Find("\"from_number\"");
	if (nFromNumPos != -1) {
		int c = strJsonA.Find(':', nFromNumPos);
		CStringA numStr = strJsonA.Mid(c + 1);
		numStr.Trim();
		int endPos = numStr.FindOneOf(",}");
		if (endPos != -1) {
			numStr = numStr.Left(endPos);
			numStr.Trim();
			nFromNumber = atoi(numStr);
		}
	}

	int nText = strJsonA.Find("\"text\": \"");
	if (nText != -1) {
		CStringA sText = strJsonA.Mid(nText + 9);
		sText = sText.Left(sText.Find('\"'));

		CString msg;
		// Player{number} 형식으로 표시
		msg.Format(_T("Player%d: %s"), nFromNumber, (LPCTSTR)CStrA_to_CStr(sText));
		AppendTextToRichEdit(msg);
	}
}

// ★ [수정] 최신 정보를 받아 리스트를 갱신하는 함수
void CDayDlg::ParseRoomState(const CStringA& strJsonA)
{
	m_vecDayPlayers.clear();

	const char* pData = strJsonA.GetString();
	const char* pPlayer = strstr(pData, "\"uid\": \"");

	while (pPlayer)
	{
		const char* pObjEnd = strchr(pPlayer, '}');
		if (!pObjEnd) break;

		CStringA strPlayerObj(pPlayer, pObjEnd - pPlayer + 1);

		// 공백 제거 (파싱 안전장치)
		CStringA strCleanObj = strPlayerObj;
		strCleanObj.Replace(" ", "");
		strCleanObj.Replace("\t", "");
		strCleanObj.Replace("\r", "");
		strCleanObj.Replace("\n", "");

		// 데이터 추출
		CStringA strUid = "";
		int kUid = strPlayerObj.Find("\"uid\"");
		if (kUid != -1) {
			int c = strPlayerObj.Find(':', kUid);
			int s = strPlayerObj.Find('\"', c + 1);
			int e = strPlayerObj.Find('\"', s + 1);
			if (s != -1 && e != -1) strUid = strPlayerObj.Mid(s + 1, e - s - 1);
		}

		CStringA strName = "";
		int kName = strPlayerObj.Find("\"name\"");
		if (kName != -1) {
			int c = strPlayerObj.Find(':', kName);
			int s = strPlayerObj.Find('\"', c + 1);
			int e = strPlayerObj.Find('\"', s + 1);
			if (s != -1 && e != -1) strName = strPlayerObj.Mid(s + 1, e - s - 1);
		}

		// Player Number 추출
		int nPlayerNumber = 0;

		// 방법 1: "number" 필드에서 추출
		int kNumber = strPlayerObj.Find("\"number\"");
		if (kNumber != -1) {
			int c = strPlayerObj.Find(':', kNumber);
			CStringA numStr = strPlayerObj.Mid(c + 1);
			numStr.Trim();
			int endPos = numStr.FindOneOf(",}");
			if (endPos != -1) {
				numStr = numStr.Left(endPos);
				numStr.Trim();
				nPlayerNumber = atoi(numStr);
			}
		}

		// 방법 2: "number" 필드가 없으면 "name"에서 "Player X" 형식 파싱
		if (nPlayerNumber == 0 && !strName.IsEmpty()) {
			int playerPos = strName.Find("Player");
			if (playerPos != -1) {
				CStringA numPart = strName.Mid(playerPos + 6); // "Player" 다음부터
				numPart.Trim();
				if (!numPart.IsEmpty()) {
					nPlayerNumber = atoi(numPart);
				}
			}
		}

		bool bAlive = (strCleanObj.Find("\"alive\":true") != -1);
		bool bIsHost = (strCleanObj.Find("\"is_host\":true") != -1);

		RoomPlayerInfo player;
		player.strUID = CStrA_to_CStr(strUid);
		player.strName = CStrA_to_CStr(strName);
		player.nPlayerNumber = nPlayerNumber;  // 플레이어 번호 저장
		player.bIsAlive = bAlive;
		player.bIsHost = bIsHost;

		m_vecDayPlayers.push_back(player);

		pPlayer = strstr(pObjEnd, "\"uid\": \"");
	}

	// [중요] 메인 화면에도 업데이트 (다음 밤을 위해)
	CMafia43Dlg* pMain = dynamic_cast<CMafia43Dlg*>(GetParent());
	if (pMain)
	{
		pMain->m_vecRoomPlayers = m_vecDayPlayers;
	}

	// ★ [중요] 화면 새로고침
	PopulateVoteList();
}

void CDayDlg::ParseVoteResult(const CStringA& strJsonA) {}

CStringA CDayDlg::CStr_to_CStrA(const CString& strT) { CT2A utf8(strT, CP_UTF8); return CStringA(utf8); }
CString CDayDlg::CStrA_to_CStr(const CStringA& strA) { CA2T utf8(strA, CP_UTF8); return CString(utf8); }