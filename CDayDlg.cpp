#include "pch.h"
#include "Mafia43.h"   // 메인 앱 헤더
#include "CDayDlg.h"
#include "resource.h"    // 리소스 헤더
#include "Mafia43Dlg.h"

IMPLEMENT_DYNAMIC(CDayDlg, CDialogEx)

CDayDlg::CDayDlg(CWnd* pParent /*=nullptr*/, CClientSocket* pSocket /*=nullptr*/,
	CString strMyUID /*= _T("")*/, CString strMyNickname /*= _T("")*/, CString strMyRole /*= _T("")*/,
	const std::vector<RoomPlayerInfo>& players /*= std::vector<RoomPlayerInfo>()*/)
	: CDialogEx(IDD_DAY, pParent)
	, m_pSocket(pSocket)
	, m_strMyUID(strMyUID)
	, m_strMyNickname(strMyNickname)
	, m_strMyRole(strMyRole)
	, m_strChatMsg(_T(""))
	, m_nDayTimeLimit(180)
	, m_vecDayPlayers(players) // [핵심] 전달받은 플레이어 목록으로 초기화
{
}

CDayDlg::~CDayDlg()
{
}

void CDayDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	// [수정] IDD_DAY UI에 배치한 ID와 일치해야 합니다.
	DDX_Control(pDX, IDC_RICH_CHAT, m_richChat); // CListBox -> CRichEditCtrl
	DDX_Control(pDX, IDC_LIST_VOTE, m_listVote);
	DDX_Text(pDX, IDC_EDIT_CHAT, m_strChatMsg);
}


BEGIN_MESSAGE_MAP(CDayDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_SEND_CHAT, &CDayDlg::OnBnClickedButtonSendChat)
	ON_BN_CLICKED(IDC_BUTTON_VOTE, &CDayDlg::OnBnClickedButtonVote)
	ON_MESSAGE(WM_USER_RECV_MSG, &CDayDlg::OnReceiveMsg)
	ON_WM_TIMER() // [추가] 타이머 메시지 맵
END_MESSAGE_MAP()


// CDayDlg 메시지 처리기

BOOL CDayDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	if (m_pSocket == nullptr)
	{
		AfxMessageBox(_T("소켓 연결이 없습니다."));
		OnCancel();
		return FALSE;
	}

	// 1. [유지] 투표 목록 컬럼 설정
	m_listVote.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	m_listVote.InsertColumn(0, _T("UID"), LVCFMT_LEFT, 0); // UID 컬럼 (숨김)
	m_listVote.InsertColumn(1, _T("닉네임"), LVCFMT_LEFT, 150);
	m_listVote.InsertColumn(2, _T("상태"), LVCFMT_LEFT, 80);

	CString strRoleDisplay;
	strRoleDisplay.Format(_T("역할: %s"), m_strMyRole);
	// [추가] 텍스트 ID (IDC_STATIC)를 사용하여 역할 표시
	SetDlgItemText(IDC_STATIC, strRoleDisplay);

	// 2. [유지] 서버에 생존자 목록 요청
	// m_pSocket->SendJson("{\"op\":\"ROOM_STATE\"}");
	PopulateVoteList();

	// 3. [수정] 채팅창에 알림 (Rich Edit 방식)
	AppendTextToRichEdit(_T("[알림] 낮이 되었습니다. 토론을 시작하세요.\r\n"), RGB(0, 0, 255));

	// 4. [추가] 타이머 시작
	UpdateTimerDisplay(); // 타이머 UI 즉시 갱신
	SetTimer(1, 1000, NULL); // 타이머 ID 1번, 1초(1000ms) 간격

	return TRUE;
}

void CDayDlg::PopulateVoteList()
{
	m_listVote.DeleteAllItems();
	int nItem = 0;
	for (const auto& player : m_vecDayPlayers)
	{
		if (player.bIsAlive) // 생존자만 표시
		{
			// Column 0: UID (숨김)
			m_listVote.InsertItem(nItem, player.strUID);
			// Column 1: Nickname
			m_listVote.SetItemText(nItem, 1, player.strName);
			// Column 2: Status
			m_listVote.SetItemText(nItem, 2, _T("생존"));
			nItem++;
		}
	}
}

/**
 * @brief [추가] 타이머 핸들러 (1초마다 실행됨)
 */
void CDayDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == 1) // 1번 타이머일 경우
	{
		m_nDayTimeLimit--; // 1초 감소
		UpdateTimerDisplay(); // UI 갱신

		if (m_nDayTimeLimit <= 0)
		{
			KillTimer(1); // 타이머 중지

			// 서버가 페이즈를 넘기도록 호스트가 요청해야 함 (클라이언트가 임의로 닫지 않음)
			// (서버 코드에 자동 타이머가 없으므로, 방장(Host)이 NEXT_PHASE를 보내야 함)
			// 지금은 일단 UI만 비활성화

			GetDlgItem(IDC_BUTTON_VOTE)->EnableWindow(FALSE);
			GetDlgItem(IDC_BUTTON_SEND_CHAT)->EnableWindow(FALSE);
			AppendTextToRichEdit(_T("[알림] 토론 시간이 종료되었습니다.\r\n"), RGB(255, 0, 0));
		}
	}

	CDialogEx::OnTimer(nIDEvent);
}

/**
 * @brief [추가] 타이머 UI 갱신 헬퍼
 */
void CDayDlg::UpdateTimerDisplay()
{
	int minutes = m_nDayTimeLimit / 60;
	int seconds = m_nDayTimeLimit % 60;

	CString strTime;
	strTime.Format(_T("남은 시간: %02d:%02d"), minutes, seconds);

	// IDC_STATIC_TIME ID를 가진 Static Text 컨트롤의 글자를 변경
	SetDlgItemText(IDC_STATIC_TIME, strTime);
}

/**
 * @brief [추가] Rich Edit 컨트롤에 텍스트 추가 헬퍼
 */
void CDayDlg::AppendTextToRichEdit(CString strText, COLORREF color)
{
	//줄바꿈
	if (strText.Right(2) != _T("\r\n"))
	{
		strText += _T("\r\n");
	}

	CHARFORMAT cf;
	ZeroMemory(&cf, sizeof(CHARFORMAT));
	cf.cbSize = sizeof(CHARFORMAT);
	cf.dwMask = CFM_COLOR;
	cf.crTextColor = color;

	long nLength = m_richChat.GetWindowTextLength();
	m_richChat.SetSel(nLength, nLength);
	m_richChat.SetSelectionCharFormat(cf);
	m_richChat.ReplaceSel(strText);

	m_richChat.PostMessage(WM_VSCROLL, SB_BOTTOM, 0); // 자동 스크롤
}


// --- [아래는 기존 함수들에서 채팅창 로직만 수정됨] ---

void CDayDlg::OnBnClickedButtonSendChat()
{
	UpdateData(TRUE);
	if (m_strChatMsg.IsEmpty() || m_pSocket == nullptr) return;

	CStringA strJsonMsg;
	strJsonMsg.Format("{\"op\": \"DAY_CHAT\", \"text\": \"%s\"}",
		(LPCSTR)CStr_to_CStrA(m_strChatMsg));

	m_pSocket->SendJson(strJsonMsg);

	m_strChatMsg = _T("");
	UpdateData(FALSE);
	GetDlgItem(IDC_EDIT_CHAT)->SetFocus();
}

void CDayDlg::OnBnClickedButtonVote()
{
	POSITION pos = m_listVote.GetFirstSelectedItemPosition();
	if (pos == NULL)
	{
		AfxMessageBox(_T("투표할 대상을 선택하세요."));
		return;
	}

	int nItem = m_listVote.GetNextSelectedItem(pos);
	CString strTargetUID = m_listVote.GetItemText(nItem, 0);

	if (strTargetUID.IsEmpty() || m_pSocket == nullptr) return;

	if (strTargetUID == m_strMyUID)
	{
		AfxMessageBox(_T("자신에게 투표할 수 없습니다."));
		return;
	}

	CStringA strJsonMsg;
	strJsonMsg.Format("{\"op\": \"VOTE\", \"target\": \"%s\"}",
		(LPCSTR)CStr_to_CStrA(strTargetUID));

	m_pSocket->SendJson(strJsonMsg);

	GetDlgItem(IDC_BUTTON_VOTE)->EnableWindow(FALSE);
	AppendTextToRichEdit(_T("[알림] 투표를 완료했습니다.\r\n"), RGB(0, 0, 255));
}


afx_msg LRESULT CDayDlg::OnReceiveMsg(WPARAM wParam, LPARAM lParam)
{
	CStringA* pJsonA = (CStringA*)wParam;
	if (!pJsonA) return 0;

	CStringA strJsonA = *pJsonA;
	delete pJsonA;

	ProcessServerMessage(strJsonA);
	return 0;
}

void CDayDlg::ProcessServerMessage(CStringA strJsonA)
{
	if (strJsonA.Find("\"op\": \"CHAT\"") != -1)
	{
		ParseChat(strJsonA);
	}
	else if (strJsonA.Find("\"op\": \"VOTE_RESULT\"") != -1)
	{
		ParseVoteResult(strJsonA);
	}
	else if (strJsonA.Find("\"op\": \"ROOM_STATE\"") != -1)
	{
		ParseRoomState(strJsonA);
	}
	else if (strJsonA.Find("\"op\": \"PHASE\"") != -1)
	{
		if (strJsonA.Find("\"phase\": \"NIGHT\"") != -1)
		{
			KillTimer(1); // [추가] 다이얼로그 닫기 전 타이머 중지
			AppendTextToRichEdit(_T("[알림] 밤이 되었습니다.\r\n"), RGB(255, 0, 0));
			OnOK();
		}
	}
	else if (strJsonA.Find("\"op\": \"GAME_END\"") != -1)
	{
		KillTimer(1); // [추가] 다이얼로그 닫기 전 타이머 중지
		AfxMessageBox(_T("게임이 종료되었습니다!"));
		OnCancel();
	}
	else if (strJsonA.Find("\"op\": \"ERROR\"") != -1)
	{
		AfxMessageBox(CStrA_to_CStr(strJsonA));
	}
}

void CDayDlg::ParseChat(const CStringA& strJsonA)
{
	const char* pData = strJsonA.GetString();

	const char* pFrom = strstr(pData, "\"from\": \"");
	if (!pFrom) return;
	const char* pFromEnd = strstr(pFrom + 9, "\"");
	if (!pFromEnd) return;

	const char* pText = strstr(pFromEnd, "\"text\": \"");
	if (!pText) return;
	const char* pTextEnd = strstr(pText + 9, "\"");
	if (!pTextEnd) return;

	CStringA strFromA(pFrom + 9, pFromEnd - (pFrom + 9));
	CStringA strTextA(pText + 9, pTextEnd - (pText + 9));

	CString strMsg;
	strMsg.Format(_T("%s: %s\r\n"), CStrA_to_CStr(strFromA), CStrA_to_CStr(strTextA));
	AppendTextToRichEdit(strMsg); // [수정] m_listChat.AddString -> AppendTextToRichEdit
}

void CDayDlg::ParseVoteResult(const CStringA& strJsonA)
{
	CString strMsg = _T("[투표 결과] ") + CStrA_to_CStr(strJsonA) + _T("\r\n");
	AppendTextToRichEdit(strMsg, RGB(0, 100, 0)); // [수정]
	AfxMessageBox(strMsg);
}

void CDayDlg::ParseRoomState(const CStringA& strJsonA)
{
	// [수정] 서버에서 ROOM_STATE 메시지를 받으면 내부 목록을 갱신합니다.
	m_vecDayPlayers.clear();

	const char* pData = strJsonA.GetString();
	const char* pPlayer = strstr(pData, "\"uid\": \"");
	int nItem = 0;

	while (pPlayer)
	{
		// ... (기존 파싱 로직: strUid, strName, strAlive 추출)
		const char* pUidEnd = strstr(pPlayer + 9, "\"");
		if (!pUidEnd) { pPlayer = nullptr; continue; }

		const char* pName = strstr(pUidEnd, "\"name\": \"");
		if (!pName) { pPlayer = nullptr; continue; }

		const char* pNameEnd = strstr(pName + 10, "\"");
		if (!pNameEnd) { pPlayer = nullptr; continue; }

		const char* pAlive = strstr(pNameEnd, "\"alive\": ");
		if (!pAlive) { pPlayer = nullptr; continue; }

		const char* pAliveEnd = strstr(pAlive + 9, ",");
		if (!pAliveEnd) { pPlayer = nullptr; continue; }

		// [추가] is_host를 파싱하여 RoomPlayerInfo를 완성
		const char* pHost = strstr(pAliveEnd, "\"is_host\": ");
		if (!pHost) { pPlayer = nullptr; continue; }

		const char* pHostEnd = strstr(pHost + 11, "}");
		if (!pHostEnd) { pPlayer = nullptr; continue; }


		CStringA strUid(pPlayer + 9, pUidEnd - (pPlayer + 9));
		CStringA strName(pName + 10, pNameEnd - (pName + 10));
		CStringA strAlive(pAlive + 9, pAliveEnd - (pAlive + 9));

		// [수정] RoomPlayerInfo를 생성하여 m_vecDayPlayers에 추가
		RoomPlayerInfo player;
		player.strUID = CStrA_to_CStr(strUid);
		player.strName = CStrA_to_CStr(strName);
		player.bIsAlive = (strAlive == "true");

		// 'is_host' 필드를 포함하는 정확한 JSON 파싱이 필요하지만,
		// 현재 CDayDlg의 JSON 파싱은 이 부분을 생략하고 있으므로, 
		// 간단히 false로 설정하고 다음 플레이어를 찾습니다.
		player.bIsHost = false;
		m_vecDayPlayers.push_back(player);

		pPlayer = strstr(pHostEnd, "\"uid\": \""); // 다음 플레이어 검색
	}

	// [핵심] 내부 목록 갱신 후, 리스트 뷰 UI를 새로고침합니다.
	PopulateVoteList();
}


// --- [유지] 헬퍼 함수 ---

CStringA CDayDlg::CStr_to_CStrA(const CString& strT)
{
	CT2A utf8(strT, CP_UTF8);
	return CStringA(utf8);
}

CString CDayDlg::CStrA_to_CStr(const CStringA& strA)
{
	CA2T utf8(strA, CP_UTF8);
	return CString(utf8);
}

