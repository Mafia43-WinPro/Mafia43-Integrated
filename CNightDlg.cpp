#include "pch.h"
#include "Mafia43.h"       // 프로젝트 메인 헤더
#include "CNightDlg.h"     // 밤 화면 헤더
#include "afxdialogex.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNAMIC(CNightDlg, CDialogEx)



// --- CNightDlg 생성자 ---
CNightDlg::CNightDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_NIGHT_DIALOG, pParent)
	, m_pSocket(nullptr)
	, m_timeLeftSec(30) // 기본 30초
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CNightDlg::~CNightDlg()
{
}

void CNightDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	// UI 컨트롤 연결
	// (주의: 리소스 편집기에서 ID가 일치하지 않으면 여기서 오류가 납니다)
	DDX_Control(pDX, IDC_LIST_PLAYERS, m_playerList);
	DDX_Control(pDX, IDC_CMB_ACTION, m_cmbAction);
	DDX_Control(pDX, IDC_BTN_CONFIRM, m_btnConfirm);
	DDX_Control(pDX, IDC_LBL_ROLE, m_lblRole);
	DDX_Control(pDX, IDC_LBL_TIMER, m_lblTimer);
	DDX_Control(pDX, IDC_LBL_PREVIEW, m_lblPreview);
	DDX_Control(pDX, IDC_RE_CHATVIEW, m_chatView);
	DDX_Control(pDX, IDC_EDT_CHAT, m_chatInput);
	DDX_Control(pDX, IDC_BTN_SEND, m_btnSend);
}

BEGIN_MESSAGE_MAP(CNightDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_BTN_CONFIRM, &CNightDlg::OnBnClickedConfirm)
	ON_BN_CLICKED(IDC_BTN_SEND, &CNightDlg::OnClickedSend)
	ON_BN_CLICKED(IDC_BTN_SKIP, &CNightDlg::OnClickedSkip) // 버튼이 있다면 주석 해제
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_PLAYERS, &CNightDlg::OnItemchangedPlayerList)
	ON_EN_CHANGE(IDC_RE_CHATVIEW, &CNightDlg::OnEnChangeReChatview)
END_MESSAGE_MAP()


// --- 초기화 ---
BOOL CNightDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 시스템 메뉴 설정
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

	SetIcon(m_hIcon, TRUE);
	SetIcon(m_hIcon, FALSE);

	// 1. 리스트 컨트롤 설정
	m_playerList.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	m_playerList.InsertColumn(0, _T("이름"), LVCFMT_LEFT, 140);

	// 2. 역할 및 타이머 표시
	CString strRoleText;
	strRoleText.Format(_T("역할: %s"), m_strMyRole);
	m_lblRole.SetWindowText(strRoleText);

	m_lblTimer.SetWindowText(_T("남은 시간: 30초"));
	m_lblPreview.SetWindowText(_T("선택: 없음"));
	m_chatView.SetReadOnly(TRUE);

	// 3. 콤보박스 초기화 (내 역할에 맞는 스킬 추가)
	m_cmbAction.ResetContent();
	m_cmbAction.AddString(_T("NONE")); // 기본값: 아무것도 안 함

	// 내 역할(m_strMyRole)에 따라 스킬 목록 추가
	if (m_strMyRole == _T("마피아"))
	{
		m_cmbAction.AddString(_T("KILL")); // 마피아는 '처형'
	}
	else if (m_strMyRole == _T("의사"))
	{
		m_cmbAction.AddString(_T("SAVE")); // 의사는 '치료'
	}
	else if (m_strMyRole == _T("경찰"))
	{
		m_cmbAction.AddString(_T("CHECK")); // 경찰은 '조사'
	}

	// 기본적으로 "NONE" 선택
	m_cmbAction.SetCurSel(0);

	// 4. 더미 데이터 (나중에 서버 데이터로 교체 필요)
	// 일단 테스트를 위해 유지
	m_players = {
		{1, _T("Player1"), true},
		{2, _T("Player2"), true},
		{3, _T("Player3"), true},
		{4, _T("Player4"), true}
	};

	InitPlayerList();

	// 5. 타이머 시작
	m_timeLeftSec = 30;
	SetTimer(1, 1000, nullptr);

	return TRUE;
}

// --- 기능 구현 ---

void CNightDlg::InitPlayerList()
{
	m_playerList.DeleteAllItems();
	for (const auto& p : m_players)
	{
		if (!p.alive) continue;
		int row = m_playerList.InsertItem(m_playerList.GetItemCount(), p.name);
		m_playerList.SetItemData(row, p.id);
	}
}

void CNightDlg::AppendChat(CString strMsg)
{
	//줄바꿈
	if (strMsg.Right(2) != _T("\r\n"))
	{
		strMsg += _T("\r\n");
	}

	int nLen = m_chatView.GetWindowTextLength();
	m_chatView.SetSel(nLen, nLen);
	m_chatView.ReplaceSel(strMsg);

	m_chatView.PostMessage(WM_VSCROLL, SB_BOTTOM, 0); // 자동 스크롤
}

// --- 이벤트 핸들러 ---

void CNightDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == 1 && m_timeLeftSec > 0)
	{
		--m_timeLeftSec;
		CString s;
		s.Format(_T("남은 시간: %02d:%02d"), m_timeLeftSec / 60, m_timeLeftSec % 60);
		m_lblTimer.SetWindowText(s);

		if (m_timeLeftSec == 0)
		{
			KillTimer(1);
			AppendChat(_T("시간이 만료되어 턴이 종료됩니다.\r\n"));
			OnOK(); // 시간 종료 시 자동으로 낮으로 전환
		}
	}
	CDialogEx::OnTimer(nIDEvent);
}

// [확인] 버튼 클릭 시
// 현재 리스트에 Player1, Player2... 하드코딩(테스트용)
void CNightDlg::OnBnClickedConfirm()
{
	// 1. 콤보박스에서 선택한 행동 가져오기 (MAFIA, DOCTOR 등)
	int sel = m_cmbAction.GetCurSel();
	CString action;
	if (sel >= 0) m_cmbAction.GetLBText(sel, action);
	else action = _T("NONE");

	// 2. 리스트에서 선택한 대상의 이름(닉네임) 가져오기
	int item = m_playerList.GetNextItem(-1, LVNI_SELECTED);
	CString targetUID = _T("");

	if (item != -1) {
		// 0번 컬럼(이름)의 텍스트를 가져옵니다.
		targetUID = m_playerList.GetItemText(item, 0);
	}

	// 3. 유효성 검사 (행동을 골랐는데 대상을 안 골랐으면 막음)
	if (action != _T("NONE") && targetUID.IsEmpty()) {
		AfxMessageBox(_T("대상을 선택하세요!"));
		return;
	}

	// 4. (핵심) 서버에 '능력 사용' JSON 패킷 전송
	if (m_pSocket)
	{
		// 프로토콜: {"op": "NIGHT_ACTION", "target": "타겟이름"}
		CStringA strJson;

		// CString(유니코드) -> CStringA(ASCII/UTF8) 변환
		CT2A asciiTarget(targetUID);

		// JSON 만들기
		strJson.Format("{\"op\": \"NIGHT_ACTION\", \"target\": \"%s\"}", (LPCSTR)asciiTarget);

		// 전송!
		m_pSocket->SendJson(strJson);
	}

	// 5. 로그 표시 (내가 누굴 찍었는지 채팅창에 남기기)
	CString log;
	log.Format(_T("[시스템] '%s'님에게 능력을 사용했습니다.\r\n"), targetUID);
	AppendChat(log);

	// 6. 밤 종료 (낮으로 전환)
	// 이 함수 안에서 NEXT_PHASE도 보내고 다이얼로그도 닫습니다.
	OnOK();
}

// [건너뛰기] 버튼 클릭 시
void CNightDlg::OnClickedSkip()
{
	OnOK(); // 바로 밤 종료
}

// [전송] 버튼 클릭 시
void CNightDlg::OnClickedSend()
{
	CString msg;
	m_chatInput.GetWindowText(msg);
	msg.Trim();
	if (msg.IsEmpty()) return;

	// TODO: 밤 채팅 서버 전송 (마피아끼리만)
	CString line;
	line.Format(_T("[나] %s\r\n"), msg);
	AppendChat(line);
	m_chatInput.SetWindowText(_T(""));
}

void CNightDlg::OnItemchangedPlayerList(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLISTVIEW p = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	if ((p->uChanged & LVIF_STATE) && (p->uNewState & LVIS_SELECTED)) {
		int idx = p->iItem;
		if (idx >= 0) {
			m_selectedTargetId = (int)m_playerList.GetItemData(idx);
			CString name = m_playerList.GetItemText(idx, 0);
			CString preview;
			preview.Format(_T("선택 대상: %s"), name);
			m_lblPreview.SetWindowText(preview);
		}
	}
	*pResult = 0;
}

/**
 * @brief 밤 화면이 닫힐 때 (확인/스킵/타이머) 호출
 * 여기서 서버 상태를 '낮(DAY)'으로 바꿔달라고 요청합니다.
 */
void CNightDlg::OnOK()
{
	if (m_pSocket)
	{
		// ★ 서버 상태 변경 요청 (Night -> Day)
		m_pSocket->SendJson("{\"op\": \"NEXT_PHASE\"}");
	}

	CDialogEx::OnOK(); // 다이얼로그 닫기
}

// --- 기타 UI 핸들러 ---
void CNightDlg::OnSysCommand(UINT nID, LPARAM lParam) {
		CDialogEx::OnSysCommand(nID, lParam);
	
}

void CNightDlg::OnPaint() {
	if (IsIconic()) {
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
	else {
		CDialogEx::OnPaint();
	}
}

HCURSOR CNightDlg::OnQueryDragIcon() {
	return static_cast<HCURSOR>(m_hIcon);
}

void CNightDlg::OnEnChangeReChatview() {}

