// CNightDlg.cpp : 구현 파일

#include "pch.h"
#include "Mafia43.h"       // 프로젝트 메인 헤더
#include "CNightDlg.h"     // 밤 화면 헤더
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
	, m_timeLeftSec(60) // ★ [수정] 밤 시간 60초로 설정
	, m_players(players)
	, m_bActionSubmitted(false)
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
	ON_BN_CLICKED(IDC_BTN_SKIP, &CNightDlg::OnClickedSkip)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_PLAYERS, &CNightDlg::OnItemchangedPlayerList)
	ON_EN_CHANGE(IDC_RE_CHATVIEW, &CNightDlg::OnEnChangeReChatview)
	// ★ [필수] 서버 메시지 수신 핸들러 연결
	ON_MESSAGE(WM_USER_RECV_MSG, &CNightDlg::OnReceiveMsg)
END_MESSAGE_MAP()


// --- 초기화 ---
BOOL CNightDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 시스템 메뉴 설정 (생략 가능하지만 유지)
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
	// 이름 컬럼 (UID 대신 이름 표시)
	m_playerList.InsertColumn(0, _T("플레이어"), LVCFMT_LEFT, 140);

	// 2. 역할 및 타이머 표시
	CString strRoleText;
	strRoleText.Format(_T("역할: %s"), m_strMyRole);
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
		// p.name에는 "Player1", "Player2" 등의 이름이 들어있어야 함
		int row = m_playerList.InsertItem(m_playerList.GetItemCount(), p.name);
		// SetItemData에 m_players 벡터의 인덱스 저장 (나중에 UID를 찾기 위해)
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

				// UI 비활성화
				m_btnConfirm.EnableWindow(FALSE);
				GetDlgItem(IDC_BTN_SKIP)->EnableWindow(FALSE);
				m_cmbAction.EnableWindow(FALSE);
				m_playerList.EnableWindow(FALSE);
			}
			OnOK(); // 다음 단계 요청
		}
	}
	CDialogEx::OnTimer(nIDEvent);
}

// CNightDlg.cpp

void CNightDlg::OnBnClickedConfirm()
{
	if (m_bActionSubmitted) return;

	// 1. 선택한 행동 가져오기
	int sel = m_cmbAction.GetCurSel();
	CString action;
	if (sel >= 0) m_cmbAction.GetLBText(sel, action);
	else action = _T("NONE");

	// 2. 리스트에서 선택한 대상 확인
	int item = m_playerList.GetNextItem(-1, LVNI_SELECTED);
	CString targetUID = _T("");
	CString targetName = _T("");

	// 리스트에서 선택한 항목의 인덱스를 가져와서 UID 찾기
	if (item != -1) {
		DWORD_PTR index = m_playerList.GetItemData(item);
		if (index < m_players.size()) {
			targetUID = m_players[index].strUID;  // 서버로 보낼 UID
			targetName = m_players[index].name;    // 로그 표시용
		}
	}

	if (action != _T("NONE") && targetUID.IsEmpty()) {
		AfxMessageBox(_T("대상을 선택하세요!"));
		return;
	}

	// 4. 서버 전송 및 디버깅 팝업
	if (m_pSocket)
	{
		// 서버로 UID 전송
		CStringA strJson;
		CT2A asciiTarget(targetUID);
		strJson.Format("{\"op\": \"NIGHT_ACTION\", \"target\": \"%s\"}", (LPCSTR)asciiTarget);
		m_pSocket->SendJson(strJson);
	}

	// 5. 로그 및 UI 처리 
	CString log;
	log.Format(_T("[시스템] '%s'님에게 능력을 사용했습니다.\r\n"), targetName);
	AppendChat(log);

	m_bActionSubmitted = true;
	m_btnConfirm.EnableWindow(FALSE);
	GetDlgItem(IDC_BTN_SKIP)->EnableWindow(FALSE);
	m_cmbAction.EnableWindow(FALSE);
	m_playerList.EnableWindow(FALSE);
}

void CNightDlg::OnClickedSkip()
{
	if (m_bActionSubmitted) return;

	if (m_pSocket)
	{
		CStringA strJson;
		strJson.Format("{\"op\": \"NIGHT_ACTION\", \"target\": \"NONE\"}");
		m_pSocket->SendJson(strJson);
	}

	AppendChat(_T("행동 건너뛰기 (NONE)를 제출했습니다.\r\n"));

	m_bActionSubmitted = true;
	m_btnConfirm.EnableWindow(FALSE);
	GetDlgItem(IDC_BTN_SKIP)->EnableWindow(FALSE);
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
			CString strTarget = m_playerList.GetItemText(idx, 0);
			CString preview;
			preview.Format(_T("선택 대상: %s"), strTarget);
			m_lblPreview.SetWindowText(preview);
		}
	}
	*pResult = 0;
}

void CNightDlg::OnOK()
{
	if (m_pSocket) m_pSocket->SendJson("{\"op\": \"NEXT_PHASE\"}");
	CDialogEx::OnOK();
}


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
		int nVictimNumber = 0;

		// victim UID 파싱
		int nVic = strJson.Find("\"victim\": \"");
		if (nVic != -1) {
			CStringA sVal = strJson.Mid(nVic + 11);
			sVal = sVal.Left(sVal.Find('\"'));
			strVictim = CString(CA2T(sVal)); // 희생자 UID
		}

		bool bSaved = (strJson.Find("\"saved\": true") != -1);

		// 누군가 죽었고, 의사가 못 살렸다면?
		if (!strVictim.IsEmpty() && !bSaved)
		{
			// 1) 내가 죽었는지 확인
			if (strVictim == m_strMyUID)
			{
				KillTimer(1); // 타이머 멈춤
				AfxMessageBox(_T("마피아에게 습격당해 사망했습니다... 로비로 돌아갑니다."));
				EndDialog(IDCANCEL); // ★ 로비로 강제 퇴장
				return 0;
			}
			else
			{
				// 2) 다른 사람이 죽었음 -> 내 내부 데이터에서 그 사람을 '사망' 처리
				for (auto& p : m_players)
				{
					if (p.strUID == strVictim) // UID로 비교
					{
						p.alive = false;
						break;
					}
				}

				// ★ [핵심] 리스트 새로고침! (이제 죽은 사람이 화면 목록에서 사라짐)
				InitPlayerList();

				// 채팅창 알림
				CString msg;
				msg.Format(_T("[속보] 플레이어(%s)가 습격당했습니다.\r\n"), strVictim);
				AppendChat(msg);
			}
		}
		else
		{
			// 아무도 안 죽음
			AppendChat(_T("[속보] 밤 동안 아무도 죽지 않았습니다.\r\n"));
		}
	}

	// 2. 낮으로 페이즈 전환 (내가 살았을 때만 실행됨)
	else if (strJson.Find("\"phase\": \"DAY\"") != -1)
	{
		OnOK(); // 낮 화면으로 이동
	}

	return 0;
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