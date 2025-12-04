// CNightDlg.cpp : 구현 파일

#include "pch.h"
#include "Mafia43.h"       // 프로젝트 메인 헤더
#include "CNightDlg.h"     // 밤 화면 헤더
#include "Mafia43Dlg.h"    // ★ 부모 대화상자 헤더 (필수)
#include "afxdialogex.h"
#include "resource.h"
#include "SharedStructures.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNAMIC(CNightDlg, CDialogEx)

// --- 생성자 ---
CNightDlg::CNightDlg(const std::vector<PlayerInfo>& players, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_NIGHT_DIALOG, pParent)
	, m_pSocket(nullptr)
	, m_timeLeftSec(60) // 밤 시간 60초
	, m_players(players)
	, m_bActionSubmitted(false)
	, m_bNextPhaseRequested(false)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CNightDlg::~CNightDlg()
{
}

void CNightDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
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
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_PLAYERS, &CNightDlg::OnItemchangedPlayerList)
	ON_EN_CHANGE(IDC_RE_CHATVIEW, &CNightDlg::OnEnChangeReChatview)
	ON_MESSAGE(WM_USER_RECV_MSG, &CNightDlg::OnReceiveMsg)
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
	m_playerList.InsertColumn(0, _T("플레이어"), LVCFMT_LEFT, 140);

	// 2. 역할 및 타이머 표시
	CString strRoleText;
	// ★ [수정] CString을 넘길 때 (LPCTSTR)로 형변환 필수
	strRoleText.Format(_T("역할: %s"), (LPCTSTR)m_strMyRole);
	m_lblRole.SetWindowText(strRoleText);

	m_lblTimer.SetWindowText(_T("남은 시간: 60초"));
	m_lblPreview.SetWindowText(_T("선택: 없음"));
	m_chatView.SetReadOnly(TRUE);

	// 3. 콤보박스 초기화
	m_cmbAction.ResetContent();
	m_cmbAction.AddString(_T("NONE"));

	if (m_strMyRole == _T("마피아")) m_cmbAction.AddString(_T("KILL"));
	else if (m_strMyRole == _T("의사")) m_cmbAction.AddString(_T("SAVE"));
	else if (m_strMyRole == _T("경찰")) m_cmbAction.AddString(_T("CHECK"));

	m_cmbAction.SetCurSel(0);

	// 4. 플레이어 목록 초기화
	InitPlayerList();

	// 5. 타이머 시작
	SetTimer(1, 1000, nullptr);

	return TRUE;
}

void CNightDlg::InitPlayerList()
{
	m_playerList.DeleteAllItems();
	for (size_t i = 0; i < m_players.size(); i++)
	{
		const auto& p = m_players[i];
		if (!p.alive) continue;

		// p.name은 CString이므로 그대로 사용 가능 (InsertItem은 오버로딩 되어 있음)
		int row = m_playerList.InsertItem(m_playerList.GetItemCount(), p.name);
		m_playerList.SetItemData(row, static_cast<DWORD_PTR>(i));
	}
}

void CNightDlg::AppendChat(CString strMsg)
{
	if (strMsg.Right(2) != _T("\r\n")) strMsg += _T("\r\n");
	int nLen = m_chatView.GetWindowTextLength();
	m_chatView.SetSel(nLen, nLen);
	m_chatView.ReplaceSel(strMsg);
	m_chatView.PostMessage(WM_VSCROLL, SB_BOTTOM, 0);
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

			if (!m_bActionSubmitted)
			{
				if (m_pSocket)
					m_pSocket->SendJson("{\"op\": \"NIGHT_ACTION\", \"target\": \"NONE\"}");

				m_btnConfirm.EnableWindow(FALSE);
				m_cmbAction.EnableWindow(FALSE);
				m_playerList.EnableWindow(FALSE);
			}
			RequestPhaseChange(true);
		}
	}
	CDialogEx::OnTimer(nIDEvent);
}

void CNightDlg::OnBnClickedConfirm()
{
	if (m_bActionSubmitted) return;

	int sel = m_cmbAction.GetCurSel();
	CString action;
	if (sel >= 0) m_cmbAction.GetLBText(sel, action);
	else action = _T("NONE");

	int item = m_playerList.GetNextItem(-1, LVNI_SELECTED);
	CString targetUID = _T("");
	CString targetName = _T("");

	if (item != -1) {
		DWORD_PTR index = m_playerList.GetItemData(item);
		if (index < m_players.size()) {
			targetUID = m_players[index].strUID;
			targetName = m_players[index].name;
		}
	}

	if (action != _T("NONE") && targetUID.IsEmpty()) {
		AfxMessageBox(_T("대상을 선택하세요!"));
		return;
	}

	if (m_pSocket)
	{
		CStringA strJson;
		CT2A asciiTarget(targetUID);
		// JSON 전송용은 CStringA이므로 (LPCSTR) 캐스팅 (기존에 잘 되어 있었음)
		strJson.Format("{\"op\": \"NIGHT_ACTION\", \"target\": \"%s\"}", (LPCSTR)asciiTarget);
		m_pSocket->SendJson(strJson);
	}

	CString log;
	// ★ [수정] CString을 넘길 때 (LPCTSTR)로 형변환
	log.Format(_T("[시스템] '%s'님에게 능력을 사용했습니다.\r\n"), (LPCTSTR)targetName);
	AppendChat(log);

	m_bActionSubmitted = true;
	m_btnConfirm.EnableWindow(FALSE);
	m_cmbAction.EnableWindow(FALSE);
	m_playerList.EnableWindow(FALSE);
}


void CNightDlg::OnClickedSend()
{
	CString msg;
	m_chatInput.GetWindowText(msg);
	msg.Trim();
	if (msg.IsEmpty()) return;

	CString line;
	// ★ [수정] CString을 넘길 때 (LPCTSTR)로 형변환
	line.Format(_T("[나] %s\r\n"), (LPCTSTR)msg);
	AppendChat(line);
	m_chatInput.SetWindowText(_T(""));
}

void CNightDlg::OnItemchangedPlayerList(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLISTVIEW p = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	if ((p->uChanged & LVIF_STATE) && (p->uNewState & LVIS_SELECTED)) {
		int idx = p->iItem;
		if (idx >= 0) {
			CString strTarget = m_playerList.GetItemText(idx, 0);
			CString preview;
			// ★ [수정] CString을 넘길 때 (LPCTSTR)로 형변환
			preview.Format(_T("선택 대상: %s"), (LPCTSTR)strTarget);
			m_lblPreview.SetWindowText(preview);
		}
	}
	*pResult = 0;
}

void CNightDlg::OnOK()
{
	RequestPhaseChange(true);
}


// --- [핵심] 메시지 수신부 ---
LRESULT CNightDlg::OnReceiveMsg(WPARAM wParam, LPARAM lParam)
{
	CStringA* pJsonA = (CStringA*)wParam;
	if (!pJsonA) return 0;
	CStringA strJson = *pJsonA;
	delete pJsonA;

	// 1. 밤 결과 확인 (누가 죽었나?)
	if (strJson.Find("\"op\": \"NIGHT_RESULT\"") != -1)
	{
		CString strVictim = _T("");

		int nVic = strJson.Find("\"victim\": \"");
		if (nVic != -1) {
			CStringA sVal = strJson.Mid(nVic + 11);
			sVal = sVal.Left(sVal.Find('\"'));
			strVictim = CString(CA2T(sVal));
		}

		bool bSaved = (strJson.Find("\"saved\": true") != -1);

		if (!strVictim.IsEmpty() && !bSaved)
		{
			// (1) 내부 리스트 업데이트 (화면 표시용)
			for (auto& p : m_players) {
				if (p.strUID == strVictim) {
					p.alive = false;
					break;
				}
			}
			InitPlayerList(); // 리스트 갱신

			// (2) 부모(MainDlg) 데이터 동기화
			CMafia43Dlg* pMain = dynamic_cast<CMafia43Dlg*>(GetParent());
			if (pMain)
			{
				for (auto& roomPlayer : pMain->m_vecRoomPlayers) {
					if (roomPlayer.strUID == strVictim) {
						roomPlayer.bIsAlive = false;
						break;
					}
				}
			}

			// (3) 메시지 출력 및 내 사망 확인
			if (strVictim == m_strMyUID) {
				KillTimer(1);
				AfxMessageBox(_T("마피아에게 습격당해 사망했습니다."));
				EndDialog(IDABORT);
				return 0;
			}
			else {
				CString msg;
				// ★ [수정] CString을 넘길 때 (LPCTSTR)로 형변환
				msg.Format(_T("[속보] 플레이어(%s)가 습격당했습니다.\r\n"), (LPCTSTR)strVictim);
				AppendChat(msg);
			}
		}
		else
		{
			AppendChat(_T("[속보] 밤 동안 아무도 죽지 않았습니다.\r\n"));
		}
	}
	// ★★★ 게임 종료 신호 처리 ★★★
	else if (strJson.Find("\"op\": \"GAME_END\"") != -1)
	{
		KillTimer(1);
		AfxMessageBox(_T("게임이 종료되었습니다!"));
		EndDialog(IDABORT);
		return 0;
	}
	// 3. 낮으로 페이즈 전환
	else if (strJson.Find("\"phase\": \"DAY\"") != -1)
	{
		RequestPhaseChange(false);
	}

	return 0;
}

void CNightDlg::RequestPhaseChange(bool bNotifyServer)
{
	if (m_bNextPhaseRequested)
		return;

	m_bNextPhaseRequested = true;

	if (bNotifyServer && m_pSocket)
		m_pSocket->SendJson("{\"op\": \"NEXT_PHASE\"}");

	CDialogEx::OnOK();
}

BOOL CNightDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		if (pMsg->wParam == VK_RETURN) return TRUE;
		if (pMsg->wParam == VK_ESCAPE) return TRUE;
	}
	return CDialogEx::PreTranslateMessage(pMsg);
}

// UI 핸들러들
void CNightDlg::OnSysCommand(UINT nID, LPARAM lParam) { CDialogEx::OnSysCommand(nID, lParam); }
void CNightDlg::OnPaint() {
	if (IsIconic()) {
		CPaintDC dc(this); SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);
		int cxIcon = GetSystemMetrics(SM_CXICON); int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect; GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2; int y = (rect.Height() - cyIcon + 1) / 2;
		dc.DrawIcon(x, y, m_hIcon);
	}
	else CDialogEx::OnPaint();
}
HCURSOR CNightDlg::OnQueryDragIcon() { return static_cast<HCURSOR>(m_hIcon); }
void CNightDlg::OnEnChangeReChatview() {}
void CNightDlg::OnBnClickedButton2() {}