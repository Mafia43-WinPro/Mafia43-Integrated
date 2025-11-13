// Mafia43Dlg.cpp: 구현 파일
// (모든 공백(space) 문제 수정 완료)

#include "pch.h"
#include "framework.h"
#include "Mafia43.h"
#include "Mafia43Dlg.h"
#include "afxdialogex.h"
#include "CRoleAssignDlg.h" 
#include "CNightDlg.h"
#include "CDayDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 응용 프로그램 정보에 사용되는 CAboutDlg 대화 상자입니다.

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

	// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	// 구현입니다.
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CMafia43Dlg 대화 상자

CMafia43Dlg::CMafia43Dlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_MAFIA43_DIALOG, pParent)
	, m_strNickname(_T("Player")) // 기본 닉네임 설정
	, m_strRoomTitle(_T("New Room")) // 기본 방 제목 설정
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_strMyRole = _T("");
	m_strMyUID = _T("");
	m_strRoomID = _T("");
}

void CMafia43Dlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);

	// --- UI 컨트롤과 변수 연결 ---
	// (UI 편집기에서 설정한 ID와 일치해야 함)

	// '방 목록' (IDC_LIST_PLAYERS)
	DDX_Control(pDX, IDC_LIST_PLAYERS, m_listRooms);

	// '방 정보' (IDC_STATIC_ROOMCODE)
	DDX_Control(pDX, IDC_STATIC_ROOMCODE, m_staticRoomInfo);

	// '닉네임' (IDC_EDIT_NICKNAME)
	DDX_Text(pDX, IDC_EDIT_NICKNAME, m_strNickname);

	// '방 안의 플레이어 목록' (IDC_LIST_PLAYERS_IN_ROOM)
	DDX_Control(pDX, IDC_LIST_PLAYERS_IN_ROOM, m_listPlayersInRoom);

	// '방 제목' (IDC_EDIT_ROOM_TITLE)
	DDX_Text(pDX, IDC_EDIT_ROOM_TITLE, m_strRoomTitle);
}

BEGIN_MESSAGE_MAP(CMafia43Dlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_TIMER() // 타이머 메시지 맵 추가

	// --- 버튼 클릭 핸들러 연결 ---
	ON_BN_CLICKED(IDC_BUTTON_EXIT, &CMafia43Dlg::OnClickedButtonExit)
	ON_BN_CLICKED(IDC_BUTTON_START, &CMafia43Dlg::OnClickedButtonConnect)
	ON_BN_CLICKED(IDC_BTN_CREATE_ROOM, &CMafia43Dlg::OnClickedButtonCreateRoom)
	ON_BN_CLICKED(IDC_BTN_JOIN_ROOM, &CMafia43Dlg::OnClickedButtonJoinRoom)
	ON_BN_CLICKED(IDC_BTN_START_GAME, &CMafia43Dlg::OnClickedButtonStartGame)

	// --- 사용자 메시지 핸들러 연결 ---
	ON_MESSAGE(WM_USER_CONNECT_SUCCESS, &CMafia43Dlg::OnConnectSuccess)
	ON_MESSAGE(WM_USER_CONNECT_FAIL, &CMafia43Dlg::OnConnectFail)
	ON_MESSAGE(WM_USER_RECV_MSG, &CMafia43Dlg::OnRecvMsg)
	ON_MESSAGE(WM_USER_SERVER_CLOSE, &CMafia43Dlg::OnServerClose)
	ON_MESSAGE(WM_USER_GAME_START, &CMafia43Dlg::OnGameStart)
END_MESSAGE_MAP()


// CMafia43Dlg 메시지 처리기

BOOL CMafia43Dlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 시스템 메뉴에 "정보..." 메뉴 항목을 추가합니다.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);
	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	SetIcon(m_hIcon, TRUE);			// 큰 아이콘을 설정합니다.
	SetIcon(m_hIcon, FALSE);		// 작은 아이콘을 설정합니다.

	// --- [중요] 소켓 및 UI 초기화 ---
	AfxSocketInit();
	m_Socket.m_pDlg = this;

	// '방 목록' 리스트 컨트롤 초기화
	m_listRooms.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	m_listRooms.InsertColumn(0, _T("방 ID"), LVCFMT_LEFT, 60);
	m_listRooms.InsertColumn(1, _T("방 제목"), LVCFMT_LEFT, 150);
	m_listRooms.InsertColumn(2, _T("인원"), LVCFMT_LEFT, 50);
	m_listRooms.InsertColumn(3, _T("상태"), LVCFMT_LEFT, 60);

	// '방 안 플레이어' 리스트 컨트롤 초기화
	m_listPlayersInRoom.SetExtendedStyle(LVS_EX_FULLROWSELECT);
	m_listPlayersInRoom.InsertColumn(0, _T("이름"), LVCFMT_LEFT, 100);
	m_listPlayersInRoom.InsertColumn(1, _T("상태"), LVCFMT_LEFT, 50);
	m_listPlayersInRoom.InsertColumn(2, _T("방장"), LVCFMT_LEFT, 40);

	// 버튼 비활성화 (서버 접속 전)
	GetDlgItem(IDC_BTN_CREATE_ROOM)->EnableWindow(FALSE);
	GetDlgItem(IDC_BTN_JOIN_ROOM)->EnableWindow(FALSE);
	GetDlgItem(IDC_BTN_START_GAME)->EnableWindow(FALSE);

	m_staticRoomInfo.SetWindowText(_T("서버에 접속하세요."));

	return TRUE;
}

void CMafia43Dlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

void CMafia43Dlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 그리기를 위한 디바이스 컨텍스트입니다.
		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

HCURSOR CMafia43Dlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

// --- 버튼 클릭 핸들러 ---

void CMafia43Dlg::OnClickedButtonConnect()
{
	UpdateData(TRUE);
	if (m_strNickname.IsEmpty())
	{
		AfxMessageBox(_T("닉네임을 입력하세요."));
		return;
	}
	if (m_Socket.m_hSocket == INVALID_SOCKET)
	{
		if (!m_Socket.Create())
		{
			AfxMessageBox(_T("소켓 생성 실패"));
			return;
		}
	}
	if (!m_Socket.Connect(_T("127.0.0.1"), 5566))
	{
		if (GetLastError() != WSAEWOULDBLOCK)
		{
			AfxMessageBox(_T("서버 연결 실패 (즉시 거부)"));
		}
	}
	m_staticRoomInfo.SetWindowText(_T("서버 연결 시도 중..."));
	GetDlgItem(IDC_BUTTON_START)->EnableWindow(FALSE);
}

void CMafia43Dlg::OnClickedButtonCreateRoom()
{
	UpdateData(TRUE);
	if (m_strRoomTitle.IsEmpty())
	{
		AfxMessageBox(_T("방 제목을 입력하세요."));
		return;
	}
	CStringA strMsg;
	strMsg.Format("{\"op\":\"CREATE_ROOM\", \"title\":\"%s\", \"max\":10}", (LPCSTR)CStr_to_CStrA(m_strRoomTitle));
	m_Socket.SendJson(strMsg);
	GetDlgItem(IDC_BTN_CREATE_ROOM)->EnableWindow(FALSE);
	GetDlgItem(IDC_BTN_JOIN_ROOM)->EnableWindow(FALSE);
}

void CMafia43Dlg::OnClickedButtonJoinRoom()
{
	int nSel = m_listRooms.GetNextItem(-1, LVNI_SELECTED);
	if (nSel == -1)
	{
		AfxMessageBox(_T("입장할 방을 선택하세요."));
		return;
	}
	CString strRoomID = m_listRooms.GetItemText(nSel, 0);
	CStringA strMsg;
	strMsg.Format("{\"op\":\"JOIN_ROOM\", \"room_id\":\"%s\"}", (LPCSTR)CStr_to_CStrA(strRoomID));
	m_Socket.SendJson(strMsg);
	GetDlgItem(IDC_BTN_CREATE_ROOM)->EnableWindow(FALSE);
	GetDlgItem(IDC_BTN_JOIN_ROOM)->EnableWindow(FALSE);
}

void CMafia43Dlg::OnClickedButtonStartGame()
{
	m_Socket.SendJson("{\"op\":\"START\"}");
	GetDlgItem(IDC_BTN_START_GAME)->EnableWindow(FALSE);
}

void CMafia43Dlg::OnClickedButtonExit()
{
	m_Socket.Close();
	OnOK();
}

// --- 사용자 메시지 핸들러 ---

LRESULT CMafia43Dlg::OnConnectSuccess(WPARAM wParam, LPARAM lParam)
{
	m_staticRoomInfo.SetWindowText(_T("서버 접속 성공."));
	return 0;
}

LRESULT CMafia43Dlg::OnConnectFail(WPARAM wParam, LPARAM lParam)
{
	m_staticRoomInfo.SetWindowText(_T("서버 연결 실패."));
	GetDlgItem(IDC_BUTTON_START)->EnableWindow(TRUE);
	return 0;
}

LRESULT CMafia43Dlg::OnServerClose(WPARAM wParam, LPARAM lParam)
{
	m_staticRoomInfo.SetWindowText(_T("서버 연결 끊김. 재접속하세요."));
	GetDlgItem(IDC_BUTTON_START)->EnableWindow(TRUE);
	GetDlgItem(IDC_BTN_CREATE_ROOM)->EnableWindow(FALSE);
	GetDlgItem(IDC_BTN_JOIN_ROOM)->EnableWindow(FALSE);
	GetDlgItem(IDC_BTN_START_GAME)->EnableWindow(FALSE);
	return 0;
}

LRESULT CMafia43Dlg::OnRecvMsg(WPARAM wParam, LPARAM lParam)
{
	CStringA* pJsonA = (CStringA*)wParam;
	CStringA strJsonA = *pJsonA;

	// --- [수정] 디버깅용 팝업 코드는 이제 삭제합니다 ---
	// AfxMessageBox(CStrA_to_CStr(strJsonA)); 

	delete pJsonA;
	ProcessServerMessage(strJsonA);
	return 0;
}

// --- [수정] JSON 파싱 로직 (공백 대응) ---

void CMafia43Dlg::ProcessServerMessage(CStringA strJsonA)
{
	// "op": "HELLO" (공백 포함)
	if (strJsonA.Find("\"op\": \"HELLO\"") != -1)
	{
		ParseHello(strJsonA);
	}
	else if (strJsonA.Find("\"op\": \"ROOM_LIST\"") != -1)
	{
		ParseRoomList(strJsonA);
	}
	else if (strJsonA.Find("\"op\": \"ROOM_STATE\"") != -1)
	{
		ParseRoomState(strJsonA);
	}
	else if (strJsonA.Find("\"op\": \"ROLE\"") != -1)
	{
		ParseRole(strJsonA);
	}
	else if (strJsonA.Find("\"op\": \"PHASE\"") != -1)
	{
		if (strJsonA.Find("\"phase\": \"NIGHT\"") != -1)
		{
			PostMessage(WM_USER_GAME_START);
		}
	}
	else if (strJsonA.Find("\"op\": \"ERROR\"") != -1)
	{
		GetDlgItem(IDC_BTN_CREATE_ROOM)->EnableWindow(TRUE);
		GetDlgItem(IDC_BTN_JOIN_ROOM)->EnableWindow(TRUE);
		AfxMessageBox(CStrA_to_CStr(strJsonA));
	}
}

void CMafia43Dlg::ParseHello(const CStringA& strJsonA)
{
	// "uid": "C2C8AB" (공백 포함)
	int nPos = strJsonA.Find("\"uid\": \"");
	if (nPos != -1)
	{
		CStringA strUid = strJsonA.Mid(nPos + 9); // (7 -> 9 수정)
		strUid = strUid.Left(strUid.Find('\"'));
		m_strMyUID = CStrA_to_CStr(strUid);

		m_strNickname = m_strMyUID;
		UpdateData(FALSE); // 컨트롤에 닉네임 표시

		m_staticRoomInfo.SetWindowText(_T("서버 접속 완료. 방을 선택하세요."));

		GetDlgItem(IDC_BTN_CREATE_ROOM)->EnableWindow(TRUE);
		GetDlgItem(IDC_BTN_JOIN_ROOM)->EnableWindow(TRUE);
	}
}

void CMafia43Dlg::ParseRoomList(const CStringA& strJsonA)
{
	m_listRooms.DeleteAllItems();
	const char* pData = strJsonA.GetString();

	// "id": "R123" (공백 포함)
	const char* pRoom = strstr(pData, "\"id\": \""); // (공백 추가)
	int nItem = 0;

	while (pRoom)
	{
		// (모든 strstr 오프셋과 길이에 공백 1~2칸 추가)
		const char* pIdEnd = strstr(pRoom + 8, "\""); // (6 -> 8)
		if (!pIdEnd) { pRoom = nullptr; continue; }

		const char* pTitle = strstr(pIdEnd, "\"title\": \""); // (공백 추가)
		if (!pTitle) { pRoom = nullptr; continue; }

		const char* pTitleEnd = strstr(pTitle + 11, "\""); // (9 -> 11)
		if (!pTitleEnd) { pRoom = nullptr; continue; }

		const char* pCur = strstr(pTitleEnd, "\"cur\": "); // (공백 추가)
		if (!pCur) { pRoom = nullptr; continue; }

		const char* pCurEnd = strstr(pCur + 7, ","); // (6 -> 7)
		if (!pCurEnd) { pRoom = nullptr; continue; }

		const char* pMax = strstr(pCurEnd, "\"max\": "); // (공백 추가)
		if (!pMax) { pRoom = nullptr; continue; }

		const char* pMaxEnd = strstr(pMax + 7, ","); // (6 -> 7)
		if (!pMaxEnd) { pRoom = nullptr; continue; }

		const char* pState = strstr(pMaxEnd, "\"state\": \""); // (공백 추가)
		if (!pState) { pRoom = nullptr; continue; }

		const char* pStateEnd = strstr(pState + 11, "\""); // (9 -> 11)
		if (!pStateEnd) { pRoom = nullptr; continue; }

		CStringA strId(pRoom + 8, pIdEnd - (pRoom + 8)); // (6 -> 8)
		CStringA strTitle(pTitle + 11, pTitleEnd - (pTitle + 11)); // (9 -> 11)
		CStringA strCur(pCur + 7, pCurEnd - (pCur + 7)); // (6 -> 7)
		CStringA strMax(pMax + 7, pMaxEnd - (pMax + 7)); // (6 -> 7)
		CStringA strState(pState + 11, pStateEnd - (pState + 11)); // (9 -> 11)

		CString strCurMax;
		strCurMax.Format(_T("%s/%s"), (LPCTSTR)CStrA_to_CStr(strCur), (LPCTSTR)CStrA_to_CStr(strMax));

		m_listRooms.InsertItem(nItem, CStrA_to_CStr(strId));
		m_listRooms.SetItemText(nItem, 1, CStrA_to_CStr(strTitle));
		m_listRooms.SetItemText(nItem, 2, strCurMax);
		m_listRooms.SetItemText(nItem, 3, CStrA_to_CStr(strState));
		nItem++;

		pRoom = strstr(pStateEnd, "\"id\": \""); // (공백 추가)
	}
}

void CMafia43Dlg::ParseRoomState(const CStringA& strJsonA)
{
	m_listPlayersInRoom.DeleteAllItems();

	// "room_id": "R123" (공백 포함)
	int nPos = strJsonA.Find("\"room_id\": \""); // (공백 추가)
	if (nPos != -1)
	{
		CStringA strRid = strJsonA.Mid(nPos + 13); // (11 -> 13)
		strRid = strRid.Left(strRid.Find('\"'));
		m_strRoomID = CStrA_to_CStr(strRid);
		m_staticRoomInfo.SetWindowText(_T("방 입장 완료: ") + m_strRoomID);
	}

	const char* pData = strJsonA.GetString();
	// "uid": "C2C8AB" (공백 포함)
	const char* pPlayer = strstr(pData, "\"uid\": \""); // (공백 추가)
	int nItem = 0;

	while (pPlayer)
	{
		// (모든 strstr 오프셋과 길이에 공백 1~2칸 추가)
		const char* pUidEnd = strstr(pPlayer + 9, "\""); // (7 -> 9)
		if (!pUidEnd) { pPlayer = nullptr; continue; }

		const char* pName = strstr(pUidEnd, "\"name\": \""); // (공백 추가)
		if (!pName) { pPlayer = nullptr; continue; }

		const char* pNameEnd = strstr(pName + 10, "\""); // (8 -> 10)
		if (!pNameEnd) { pPlayer = nullptr; continue; }

		const char* pAlive = strstr(pNameEnd, "\"alive\": "); // (공백 추가)
		if (!pAlive) { pPlayer = nullptr; continue; }

		const char* pAliveEnd = strstr(pAlive + 9, ","); // (8 -> 9)
		if (!pAliveEnd) { pPlayer = nullptr; continue; }

		const char* pHost = strstr(pAliveEnd, "\"is_host\": "); // (공백 추가)
		if (!pHost) { pPlayer = nullptr; continue; }

		const char* pHostEnd = strstr(pHost + 11, "}"); // (10 -> 11)
		if (!pHostEnd) { pPlayer = nullptr; continue; }

		CStringA strUid(pPlayer + 9, pUidEnd - (pPlayer + 9)); // (7 -> 9)
		CStringA strName(pName + 10, pNameEnd - (pName + 10)); // (8 -> 10)
		CStringA strAlive(pAlive + 9, pAliveEnd - (pAlive + 9)); // (8 -> 9)
		CStringA strHost(pHost + 11, pHostEnd - (pHost + 11)); // (10 -> 11)

		m_listPlayersInRoom.InsertItem(nItem, CStrA_to_CStr(strName));
		m_listPlayersInRoom.SetItemText(nItem, 1, (strAlive == "true" ? _T("생존") : _T("사망")));
		m_listPlayersInRoom.SetItemText(nItem, 2, (strHost == "true" ? _T("★") : _T("")));

		if (CStrA_to_CStr(strUid) == m_strMyUID && strHost == "true")
		{
			GetDlgItem(IDC_BTN_START_GAME)->EnableWindow(TRUE); // 내가 방장!
		}

		nItem++;
		pPlayer = strstr(pHostEnd, "\"uid\": \""); // (공백 추가)
	}
}

void CMafia43Dlg::ParseRole(const CStringA& strJsonA)
{
	// "role": "COP" (공백 포함)
	int nPos = strJsonA.Find("\"role\": \""); // (공백 추가)
	if (nPos != -1)
	{
		CStringA strRole = strJsonA.Mid(nPos + 10); // (8 -> 10)
		strRole = strRole.Left(strRole.Find('\"'));

		if (strRole == "MAFIA") m_strMyRole = _T("마피아");
		else if (strRole == "COP") m_strMyRole = _T("경찰");
		else if (strRole == "DOCTOR") m_strMyRole = _T("의사");
		else m_strMyRole = _T("시민");
	}
}


/**
 * @brief [수정] 게임 시작 및 밤/낮 순환(Game Loop) 처리
 */
LRESULT CMafia43Dlg::OnGameStart(WPARAM wParam, LPARAM lParam)
{
	// 1. 역할 정보가 올 때까지 0.1초 대기 (기존 로직)
	if (m_strMyRole.IsEmpty())
	{
		SetTimer(1U, 100, NULL);
		return 0;
	}
	KillTimer(1U);

	// 2. 역할 배정 팝업
	CRoleAssignDlg dlgRole;
	dlgRole.m_strRoleToShow = m_strMyRole;
	dlgRole.DoModal();

	// 3. 메인 로비 숨김
	ShowWindow(SW_HIDE);

	// 4. [신규] 게임 순환(Loop) 시작
	bool bGameInProgress = true;
	while (bGameInProgress)
	{
		// --- 4-1. 밤(NIGHT) 페이즈 ---
		CNightDlg dlgNight;
		dlgNight.m_strMyNickname = m_strNickname;
		dlgNight.m_strMyRole = m_strMyRole;
		dlgNight.m_pSocket = &m_Socket;

		// [중요] 소켓이 메시지를 보낼 대상을 '밤 다이얼로그'로 설정
		m_Socket.m_pDlg = &dlgNight;

		INT_PTR nResponse = dlgNight.DoModal();

		// OnCancel() (게임 종료 메시지 수신 등)로 닫히면 루프 종료
		if (nResponse != IDOK)
		{
			bGameInProgress = false;
			break;
		}

		// --- 4-2. 낮(DAY) 페이즈 ---
		CDayDlg dlgDay;
		dlgDay.m_pSocket = &m_Socket;
		dlgDay.m_strMyUID = m_strMyUID; // UID 전달
		dlgDay.m_strMyNickname = m_strNickname;
		dlgDay.m_strMyRole = m_strMyRole;

		// [중요] 소켓이 메시지를 보낼 대상을 '낮 다이얼로그'로 설정
		m_Socket.m_pDlg = &dlgDay;

		nResponse = dlgDay.DoModal();

		// OnCancel() (게임 종료 메시지 수신 등)로 닫히면 루프 종료
		if (nResponse != IDOK)
		{
			bGameInProgress = false;
			break;
		}
	} // end of while(bGameInProgress)

	// 5. 게임 종료 후 뒷정리

	// [중요] 소켓이 메시지를 보낼 대상을 다시 '메인 로비'로 복구
	m_Socket.m_pDlg = this;

	ShowWindow(SW_SHOW); // 메인 로비 다시 표시

	// 변수 초기화
	m_strMyRole = _T("");
	m_strRoomID = _T("");
	m_staticRoomInfo.SetWindowText(_T("게임 종료. 방을 선택하세요."));

	// 버튼 활성화
	GetDlgItem(IDC_BTN_CREATE_ROOM)->EnableWindow(TRUE);
	GetDlgItem(IDC_BTN_JOIN_ROOM)->EnableWindow(TRUE);
	GetDlgItem(IDC_BTN_START_GAME)->EnableWindow(FALSE);

	// 방 목록 갱신
	m_Socket.SendJson("{\"op\":\"LIST_ROOMS\"}");

	return 0;
}

void CMafia43Dlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == 1U)
	{
		OnGameStart(0, 0);
	}
	CDialogEx::OnTimer(nIDEvent);
}

CStringA CMafia43Dlg::CStr_to_CStrA(const CString& strT)
{
	CT2A utf8(strT, CP_UTF8);
	return CStringA(utf8);
}

CString CMafia43Dlg::CStrA_to_CStr(const CStringA& strA)
{
	CA2T utf8(strA, CP_UTF8);
	return CString(utf8);
}