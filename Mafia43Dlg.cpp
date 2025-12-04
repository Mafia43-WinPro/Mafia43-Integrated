// Mafia43Dlg.cpp: 구현 파일

#include "pch.h"
#include "framework.h"
#include "Mafia43.h"
#include "Mafia43Dlg.h"
#include "afxdialogex.h"
#include "SharedStructures.h"
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
	EnsureRoomListTimer();
	return TRUE;
}

void CMafia43Dlg::EnsureRoomListTimer()
{
	// 로비에서 방 목록을 주기적으로 요청하기 위한 타이머를 보장한다.
	// 이미 설정되어 있어도 동일한 ID로 다시 설정하면 주기가 갱신된다.
	SetTimer(2U, 2000, NULL);
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
	if (!m_Socket.Connect(_T("10.21.36.44"), 5566))
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
	m_bConnected = true;
	EnsureRoomListTimer();
	return 0;
}

LRESULT CMafia43Dlg::OnConnectFail(WPARAM wParam, LPARAM lParam)
{
	m_staticRoomInfo.SetWindowText(_T("서버 연결 실패."));
	m_bConnected = false;
	GetDlgItem(IDC_BUTTON_START)->EnableWindow(TRUE);
	return 0;
}

LRESULT CMafia43Dlg::OnServerClose(WPARAM wParam, LPARAM lParam)
{
	m_staticRoomInfo.SetWindowText(_T("서버 연결 끊김. 재접속하세요."));
	m_bConnected = false;
	m_strRoomID.Empty();
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

			//m_strNickname = m_strMyUID;
			UpdateData(FALSE);


			m_staticRoomInfo.SetWindowText(_T("서버 접속 완료. 방을 선택하세요."));

			GetDlgItem(IDC_BTN_CREATE_ROOM)->EnableWindow(TRUE);
			GetDlgItem(IDC_BTN_JOIN_ROOM)->EnableWindow(TRUE);

			UpdateData(TRUE); // 에디트 컨트롤의 값을 변수(m_strNickname)로 가져옴
			if (!m_strNickname.IsEmpty())
			{
				CStringA strPacket;
				// JSON 포맷: {"op": "LOGIN", "name": "사용자입력닉네임"}
				strPacket.Format("{\"op\":\"LOGIN\", \"name\":\"%s\"}", (LPCSTR)CStr_to_CStrA(m_strNickname));
				m_Socket.SendJson(strPacket);
			}
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

		CStringA strId(pRoom + 7, static_cast<int>(pIdEnd - (pRoom + 7)));
		CStringA strTitle(pTitle + 10, static_cast<int>(pTitleEnd - (pTitle + 10)));
		CStringA strCur(pCur + 7, static_cast<int>(pCurEnd - (pCur + 7)));
		CStringA strMax(pMax + 7, static_cast<int>(pMaxEnd - (pMax + 7)));
		CStringA strState(pState + 10, static_cast<int>(pStateEnd - (pState + 10)));

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

		// 2-1) [추가] Player Number 추출
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
			// "Player 17" 형식에서 숫자 추출
			int playerPos = strName.Find("Player");
			if (playerPos != -1) {
				CStringA numPart = strName.Mid(playerPos + 6); // "Player" 다음부터
				numPart.Trim(); // 공백 제거
				if (!numPart.IsEmpty()) {
					nPlayerNumber = atoi(numPart);
				}
			}
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
		player.nPlayerNumber = nPlayerNumber;  // 플레이어 번호 저장
		player.bIsAlive = (strAlive == "true");
		player.bIsHost = (strIsHost == "true");
		m_vecRoomPlayers.push_back(player);

		// --- 리스트 추가 --- (로비 화면 리스트 업데이트)
		// Player{number} 형식으로 표시
		CString strDisplayName;
		strDisplayName.Format(_T("Player%d"), nPlayerNumber);
		m_listPlayersInRoom.InsertItem(nItem, strDisplayName);
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

		// 방 상태를 받은 직후에도 최신 로비 정보를 요청하여
		// 방 안에 있을 때도 로비의 방/인원 수가 계속 갱신되도록 한다.
		if (m_bConnected)
		{
			m_Socket.SendJson("{\"op\":\"LIST_ROOMS\"}");
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


// CMafia43Dlg.cpp

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
		// ---------------------------------------------------------
		// [밤 페이즈 시작]
		// ---------------------------------------------------------

		// 1. 현재(최신) 생존자 목록을 바탕으로 밤 화면용 데이터 생성
		std::vector<PlayerInfo> nightPlayers;
		for (const auto& roomPlayer : m_vecRoomPlayers)
		{
			// 살아있는 사람만 밤 화면 리스트에 추가
			if (roomPlayer.bIsAlive)
			{
				PlayerInfo nightPlayer;
				nightPlayer.id = roomPlayer.nPlayerNumber;
				nightPlayer.nPlayerNumber = roomPlayer.nPlayerNumber;
				nightPlayer.strUID = roomPlayer.strUID;

				CString strDisplayName;
				strDisplayName.Format(_T("Player%d"), roomPlayer.nPlayerNumber);
				nightPlayer.name = strDisplayName;
				nightPlayer.alive = true;
				nightPlayers.push_back(nightPlayer);
			}
		}

		// 2. 밤 다이얼로그 실행
		CNightDlg dlgNight(nightPlayers, this);

		dlgNight.m_strMyNickname = m_strNickname;
		dlgNight.m_strMyRole = m_strMyRole;
		dlgNight.m_strMyUID = m_strMyUID;
		dlgNight.m_pSocket = &m_Socket;
		m_Socket.m_pDlg = &dlgNight; // 소켓 메시지를 밤 다이얼로그로 연결

		INT_PTR nResponse = dlgNight.DoModal();

		if (nResponse == IDABORT) { bGameInProgress = false; break; } // 죽거나 종료됨
		if (nResponse != IDOK) { bGameInProgress = false; break; }


		// ---------------------------------------------------------
		// [낮 페이즈 시작]
		// ---------------------------------------------------------

		// 3. 낮 다이얼로그 실행
		// (현재 m_vecRoomPlayers 정보를 생성자로 넘김)
		CDayDlg dlgDay(this, &m_Socket, m_strMyUID, m_strNickname, m_strMyRole, m_vecRoomPlayers);
		m_Socket.m_pDlg = &dlgDay; // 소켓 메시지를 낮 다이얼로그로 연결

		nResponse = dlgDay.DoModal();

		// ★★★ [핵심 수정] 낮이 끝나고 나오면, 낮 동안 변경된 정보(누가 죽었는지)를 메인 데이터에 덮어쓴다. ★★★
		// 이 코드가 없으면 다음 밤에 죽은 사람이 되살아납니다.
		m_vecRoomPlayers = dlgDay.m_vecDayPlayers;

		if (nResponse == IDABORT) { bGameInProgress = false; break; }
		if (nResponse != IDOK) { bGameInProgress = false; break; }
	}

	// 게임 종료 후 복구
	m_Socket.m_pDlg = this;
	ShowWindow(SW_SHOW);

	m_strMyRole = _T("");
	m_strRoomID = _T("");
	m_staticRoomInfo.SetWindowText(_T("게임 종료 / 로비 복귀"));

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
		// 로비/방 구분 없이 주기적으로 방 목록을 갱신한다.
		if (m_bConnected)
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

BOOL CMafia43Dlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		if (pMsg->wParam == VK_RETURN)
		{
			return TRUE; // 엔터키 무시
		}
		// ESC키도 막고 싶다면 아래 주석 해제
		
		if (pMsg->wParam == VK_ESCAPE)
		{
			return TRUE;
		}
		
	}
	return CDialogEx::PreTranslateMessage(pMsg);
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
void CMafia43Dlg::OnLvnItemchangedListPlayers(NMHDR* pNMHDR, LRESULT* pResult) { *pResult = 0; }
void CMafia43Dlg::OnStnClickedPicBackground() {}
