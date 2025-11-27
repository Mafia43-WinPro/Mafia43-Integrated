// Mafia43Dlg.cpp: 구현 파일

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
	DDX_Control(pDX, IDC_LIST_PLAYERS, m_listRooms);
	DDX_Control(pDX, IDC_STATIC_ROOMCODE, m_staticRoomInfo);
	DDX_Text(pDX, IDC_EDIT_NICKNAME, m_strNickname);
	DDX_Control(pDX, IDC_LIST_PLAYERS_IN_ROOM, m_listPlayersInRoom);
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

// [추가] 문자열 청소 함수 (공백, 따옴표, 줄바꿈 모두 제거)
CString CleanID(CString strInput)
{
	CString strResult = _T("");
	for (int i = 0; i < strInput.GetLength(); i++)
	{
		TCHAR ch = strInput.GetAt(i);
		// 숫자(0-9)거나 알파벳(A-Z, a-z)인 경우만 남김
		if ((ch >= '0' && ch <= '9') ||
			(ch >= 'A' && ch <= 'Z') ||
			(ch >= 'a' && ch <= 'z'))
		{
			strResult += ch;
		}
	}
	return strResult;
}


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
	SetTimer(2U, 2000, NULL); // [추가] 2초마다 타이머 2번 실행
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
		CPaintDC dc(this);
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

	// [주의] 서버 컴퓨터의 IP 주소를 입력해야 합니다. (로컬 테스트 시 127.0.0.1)
	// 기존에 입력하신 IP: 10.21.32.245
	if (!m_Socket.Connect(_T("10.21.32.1"), 5566))
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
	SetTimer(3U, 1000, NULL);
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
	// 디버깅: 방에 있는지 확인
	if (m_strRoomID.IsEmpty())
	{
		AfxMessageBox(_T("[디버그] 방에 입장하지 않았습니다!"));
		return;
	}

	// 디버깅: 전송할 메시지 확인
	CString strDebug;
	strDebug.Format(_T("[디버그] 전송: {\"op\":\"START\"}\n방 ID: %s"), m_strRoomID);

	AfxMessageBox(strDebug);
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

	delete pJsonA;
	ProcessServerMessage(strJsonA);
	return 0;
}

// --- JSON 파싱 로직 (수정됨) ---

void CMafia43Dlg::ProcessServerMessage(CStringA strJsonA)
{
	if (strJsonA.Find("\"op\": \"HELLO\"") != -1)
	{
		ParseHello(strJsonA);
		m_Socket.SendJson("{\"op\":\"LIST_ROOMS\"}");
	}
	else if (strJsonA.Find("\"op\": \"ROOM_LIST\"") != -1)
	{
		ParseRoomList(strJsonA);
	}
	else if (strJsonA.Find("\"op\": \"ROOM_STATE\"") != -1)
	{
		ParseRoomState(strJsonA);
		if (m_strRoomID.IsEmpty())
		{
			m_Socket.SendJson("{\"op\":\"LIST_ROOMS\"}");
		}
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
		CString strError;

		strError.Format(_T("[디버그 - 서버 에러 응답]\n원본: %s\n\n받은 시각: 방 시작 버튼 클릭 후"),

			CStrA_to_CStr(strJsonA));

		AfxMessageBox(strError);
		m_Socket.SendJson("{\"op\":\"LIST_ROOMS\"}");
	}
}

void CMafia43Dlg::ParseHello(const CStringA& strJsonA)
{
	// "uid" 키워드 찾기
	int nPos = strJsonA.Find("\"uid\"");
	if (nPos != -1)
	{
		// 콜론(:) 찾기
		int nColon = strJsonA.Find(':', nPos);

		// 값의 시작 따옴표(")와 끝 따옴표(") 찾기
		int nStart = strJsonA.Find('\"', nColon + 1);
		int nEnd = strJsonA.Find('\"', nStart + 1);

		if (nStart != -1 && nEnd != -1)
		{
			// 정확한 ID 추출
			CStringA strUid = strJsonA.Mid(nStart + 1, nEnd - nStart - 1);
			m_strMyUID = CStrA_to_CStr(strUid);
			m_strMyUID.Trim(); // 혹시 모를 공백 제거

			m_strNickname = m_strMyUID;
			UpdateData(FALSE);

			m_staticRoomInfo.SetWindowText(_T("서버 접속 완료. 방을 선택하세요."));

			GetDlgItem(IDC_BTN_CREATE_ROOM)->EnableWindow(TRUE);
			GetDlgItem(IDC_BTN_JOIN_ROOM)->EnableWindow(TRUE);
		}
	}
}

void CMafia43Dlg::ParseRoomList(const CStringA& strJsonA)
{
	m_listRooms.DeleteAllItems();
	const char* pData = strJsonA.GetString();

	const char* pRoom = strstr(pData, "\"id\": \"");
	int nItem = 0;

	while (pRoom)
	{
		// [수정] 8 -> 7
		const char* pIdEnd = strstr(pRoom + 7, "\"");
		if (!pIdEnd) { pRoom = nullptr; continue; }

		const char* pTitle = strstr(pIdEnd, "\"title\": \"");
		if (!pTitle) { pRoom = nullptr; continue; }
		// [수정] 11 -> 10
		const char* pTitleEnd = strstr(pTitle + 10, "\"");
		if (!pTitleEnd) { pRoom = nullptr; continue; }

		const char* pCur = strstr(pTitleEnd, "\"cur\": ");
		if (!pCur) { pRoom = nullptr; continue; }
		const char* pCurEnd = strstr(pCur + 7, ",");
		if (!pCurEnd) { pRoom = nullptr; continue; }

		const char* pMax = strstr(pCurEnd, "\"max\": ");
		if (!pMax) { pRoom = nullptr; continue; }
		const char* pMaxEnd = strstr(pMax + 7, ",");
		if (!pMaxEnd) { pRoom = nullptr; continue; }

		const char* pState = strstr(pMaxEnd, "\"state\": \"");
		if (!pState) { pRoom = nullptr; continue; }
		// [수정] 11 -> 10
		const char* pStateEnd = strstr(pState + 10, "\"");
		if (!pStateEnd) { pRoom = nullptr; continue; }

		CStringA strId(pRoom + 7, pIdEnd - (pRoom + 7));
		CStringA strTitle(pTitle + 10, pTitleEnd - (pTitle + 10));
		CStringA strCur(pCur + 7, pCurEnd - (pCur + 7));
		CStringA strMax(pMax + 7, pMaxEnd - (pMax + 7));
		CStringA strState(pState + 10, pStateEnd - (pState + 10));

		CString strCurMax;
		strCurMax.Format(_T("%s/%s"), (LPCTSTR)CStrA_to_CStr(strCur), (LPCTSTR)CStrA_to_CStr(strMax));

		m_listRooms.InsertItem(nItem, CStrA_to_CStr(strId));
		m_listRooms.SetItemText(nItem, 1, CStrA_to_CStr(strTitle));
		m_listRooms.SetItemText(nItem, 2, strCurMax);
		m_listRooms.SetItemText(nItem, 3, CStrA_to_CStr(strState));
		nItem++;

		pRoom = strstr(pStateEnd, "\"id\": \"");
	}
}

void CMafia43Dlg::ParseRoomState(const CStringA& strJsonA)
{
	m_listPlayersInRoom.DeleteAllItems();

	// [수정] 새 정보를 파싱하기 전에 기존 목록을 초기화합니다.
	m_vecRoomPlayers.clear();

	// 1. 방 ID 파싱 (기존 로직 유지)
	int nPos = strJsonA.Find("\"room_id\"");
	if (nPos != -1)
	{
		int nColon = strJsonA.Find(':', nPos);
		int nStart = strJsonA.Find('\"', nColon + 1);
		int nEnd = strJsonA.Find('\"', nStart + 1);
		if (nStart != -1 && nEnd != -1)
		{
			CStringA strRid = strJsonA.Mid(nStart + 1, nEnd - nStart - 1);
			m_strRoomID = CStrA_to_CStr(strRid);
			m_staticRoomInfo.SetWindowText(_T("방 입장 완료: ") + m_strRoomID);
		}
	}

	// ★★★ [추가] host 필드에서 방장 UID 추출 (기존 로직 유지)
	CStringA strHostUID = "";
	int nHostPos = strJsonA.Find("\"host\"");
	if (nHostPos != -1)
	{
		int nColon = strJsonA.Find(':', nHostPos);
		int nStart = strJsonA.Find('\"', nColon + 1);
		int nEnd = strJsonA.Find('\"', nStart + 1);
		if (nStart != -1 && nEnd != -1)
		{
			strHostUID = strJsonA.Mid(nStart + 1, nEnd - nStart - 1);
			strHostUID.Trim();
		}
	}

	// 2. players 배열 찾기 (기존 로직 유지)
	int nListStart = strJsonA.Find("\"players\":");
	if (nListStart == -1) return;

	int nSearchPos = strJsonA.Find('[', nListStart);
	if (nSearchPos == -1) return;

	int nItem = 0;

	// 루프: 플레이어 객체 { ... } 파싱
	while (true)
	{
		int nObjStart = strJsonA.Find('{', nSearchPos);
		if (nObjStart == -1) break;
		int nObjEnd = strJsonA.Find('}', nObjStart);
		if (nObjEnd == -1) break;

		// 플레이어 한 명 데이터
		CStringA strPlayerObj = strJsonA.Mid(nObjStart, nObjEnd - nObjStart + 1);

		// ★ 공백, 줄바꿈 제거 (기존 로직 유지)
		CStringA strCleanObj = strPlayerObj;
		strCleanObj.Replace(" ", "");
		strCleanObj.Replace("\t", "");
		strCleanObj.Replace("\r", "");
		strCleanObj.Replace("\n", "");

		// 1) UID 추출 (기존 로직 유지)
		CStringA strUid = "";
		int kUid = strPlayerObj.Find("\"uid\"");
		if (kUid != -1) {
			int c = strPlayerObj.Find(':', kUid);
			int s = strPlayerObj.Find('\"', c + 1);
			int e = strPlayerObj.Find('\"', s + 1);
			if (s != -1 && e != -1) {
				strUid = strPlayerObj.Mid(s + 1, e - s - 1);
				strUid.Trim();
			}
		}

		// 2) Name 추출 (기존 로직 유지)
		CStringA strName = "";
		int kName = strPlayerObj.Find("\"name\"");
		if (kName != -1) {
			int c = strPlayerObj.Find(':', kName);
			int s = strPlayerObj.Find('\"', c + 1);
			int e = strPlayerObj.Find('\"', s + 1);
			if (s != -1 && e != -1) strName = strPlayerObj.Mid(s + 1, e - s - 1);
		}

		// 3) Alive 추출 (기존 로직 유지)
		CStringA strAlive = "false";
		if (strCleanObj.Find("\"alive\":true") != -1) strAlive = "true";

		// ★★★ 4) Is_Host 판별 (기존 로직 유지)
		CStringA strIsHost = "false";
		if (strUid == strHostUID)
		{
			strIsHost = "true";
		}

		// --- [추가] 내부 목록(m_vecRoomPlayers)에 저장 ---
		RoomPlayerInfo player;
		player.strUID = CStrA_to_CStr(strUid);
		player.strName = CStrA_to_CStr(strName);
		player.bIsAlive = (strAlive == "true");
		player.bIsHost = (strIsHost == "true");
		m_vecRoomPlayers.push_back(player);


		// --- 리스트 추가 --- (로비 화면 리스트 업데이트)
		m_listPlayersInRoom.InsertItem(nItem, CStrA_to_CStr(strName));
		m_listPlayersInRoom.SetItemText(nItem, 1, (strAlive == "true" ? _T("생존") : _T("사망")));

		// --- 방장 확인 및 버튼 활성화 --- (기존 로직 유지)
		if (strIsHost == "true")
		{
			m_listPlayersInRoom.SetItemText(nItem, 2, _T("★")); // 별표 찍기

			// ID 비교 (CleanID로 안전하게 비교)
			CString strMyClean = CleanID(m_strMyUID);
			CString strParsedClean = CleanID(CStrA_to_CStr(strUid));

			if (strMyClean == strParsedClean)
			{
				GetDlgItem(IDC_BTN_START_GAME)->EnableWindow(TRUE); // 버튼 켜기
			}
		}
		else
		{
			m_listPlayersInRoom.SetItemText(nItem, 2, _T(""));
		}

		nItem++;
		nSearchPos = nObjEnd + 1;
	}
}

void CMafia43Dlg::ParseRole(const CStringA& strJsonA)
{
	int nPos = strJsonA.Find("\"role\": \"");
	if (nPos != -1)
	{
		// [수정] 10 -> 9
		CStringA strRole = strJsonA.Mid(nPos + 9);
		strRole = strRole.Left(strRole.Find('\"'));

		if (strRole == "MAFIA") m_strMyRole = _T("마피아");
		else if (strRole == "COP") m_strMyRole = _T("경찰");
		else if (strRole == "DOCTOR") m_strMyRole = _T("의사");
		else m_strMyRole = _T("시민");
	}
}


LRESULT CMafia43Dlg::OnGameStart(WPARAM wParam, LPARAM lParam)
{
	if (m_strMyRole.IsEmpty())
	{
		SetTimer(1U, 100, NULL);
		return 0;
	}
	KillTimer(1U);

	CRoleAssignDlg dlgRole;
	dlgRole.m_strRoleToShow = m_strMyRole;
	dlgRole.DoModal();

	ShowWindow(SW_HIDE);

	bool bGameInProgress = true;
	while (bGameInProgress)
	{
		// --- 밤 페이즈 ---

		// [추가] CNightDlg에 전달할 플레이어 목록 (UID만 포함) 생성
		std::vector<PlayerInfo> nightPlayers;
		int tempIdCounter = 1;
		for (const auto& roomPlayer : m_vecRoomPlayers)
		{
			// [핵심] 살아있는 플레이어만 목록에 포함
			if (roomPlayer.bIsAlive)
			{
				PlayerInfo nightPlayer;
				// CNightDlg의 PlayerInfo 구조체에 맞춰 데이터 변환
				nightPlayer.id = tempIdCounter++;

				// [핵심] CNightDlg의 name 필드에 UID 문자열을 저장하여 리스트에 표시되도록 합니다.
				nightPlayer.name = roomPlayer.strUID;
				nightPlayer.alive = roomPlayer.bIsAlive;
				nightPlayers.push_back(nightPlayer);
			}
		}

		// [수정] CNightDlg를 새로운 생성자(nightPlayers)로 인스턴스화
		CNightDlg dlgNight(nightPlayers, this);

		// [수정] 소켓 및 역할 설정 (기존 로직 유지)
		dlgNight.m_strMyNickname = m_strNickname;
		dlgNight.m_strMyRole = m_strMyRole;
		dlgNight.m_pSocket = &m_Socket;
		m_Socket.m_pDlg = &dlgNight;

		INT_PTR nResponse = dlgNight.DoModal();

		if (nResponse != IDOK) { bGameInProgress = false; break; }

		CDayDlg dlgDay(this, &m_Socket, m_strMyUID, m_strNickname, m_strMyRole, m_vecRoomPlayers);

		/*
		// --- 낮 페이즈 ---
		CDayDlg dlgDay;
		dlgDay.m_pSocket = &m_Socket;
		dlgDay.m_strMyUID = m_strMyUID;
		dlgDay.m_strMyNickname = m_strNickname;
		dlgDay.m_strMyRole = m_strMyRole;
		*/
		m_Socket.m_pDlg = &dlgDay; // 소켓 연결 대상 변경

		nResponse = dlgDay.DoModal();

		if (nResponse != IDOK) { bGameInProgress = false; break; }
	}

	// 게임 종료 후 복구
	m_Socket.m_pDlg = this;
	ShowWindow(SW_SHOW);

	m_strMyRole = _T("");
	m_strRoomID = _T("");
	m_staticRoomInfo.SetWindowText(_T("게임 종료. 방을 선택하세요."));

	GetDlgItem(IDC_BTN_CREATE_ROOM)->EnableWindow(TRUE);
	GetDlgItem(IDC_BTN_JOIN_ROOM)->EnableWindow(TRUE);
	GetDlgItem(IDC_BTN_START_GAME)->EnableWindow(FALSE);

	m_Socket.SendJson("{\"op\":\"LIST_ROOMS\"}");

	return 0;
}

void CMafia43Dlg::OnTimer(UINT_PTR nIDEvent)
{
	// 기존 게임 시작 타이머
	if (nIDEvent == 1U)
	{
		OnGameStart(0, 0);
	}

	// [추가] 2번 타이머: 로비에 있을 때 방 목록 자동 갱신
	if (nIDEvent == 2U)
	{
		// 방에 들어가 있지 않을 때만 요청 (방 안에서는 ROOM_STATE가 오니까 필요 없음)
		if (m_strRoomID.IsEmpty())
		{
			m_Socket.SendJson("{\"op\":\"LIST_ROOMS\"}");
		}
	}

	if (nIDEvent == 3U)
	{
		KillTimer(3U);
		m_Socket.SendJson("{\"op\":\"LIST_ROOMS\"}");
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
