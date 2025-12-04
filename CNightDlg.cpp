// CNightDlg.cpp : 구현 파일

#include "pch.h"
#include "Mafia43.h"
#include "CNightDlg.h"
#include "Mafia43Dlg.h"    // ★ 필수: 부모 대화상자 헤더
#include "afxdialogex.h"
#include "resource.h"
#include "SharedStructures.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNAMIC(CNightDlg, CDialogEx)

CNightDlg::CNightDlg(const std::vector<PlayerInfo>& players, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_NIGHT_DIALOG, pParent)
	, m_pSocket(nullptr)
	, m_timeLeftSec(60)
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

BOOL CNightDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

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

	m_playerList.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	m_playerList.InsertColumn(0, _T("플레이어"), LVCFMT_LEFT, 140);

	CString strRoleText;
	strRoleText.Format(_T("역할: %s"), (LPCTSTR)m_strMyRole);
	m_lblRole.SetWindowText(strRoleText);

	m_lblTimer.SetWindowText(_T("남은 시간: 60초"));
	m_lblPreview.SetWindowText(_T("선택: 없음"));
	m_chatView.SetReadOnly(TRUE);

	m_cmbAction.ResetContent();
	m_cmbAction.AddString(_T("NONE"));

	if (m_strMyRole == _T("마피아")) m_cmbAction.AddString(_T("KILL"));
	else if (m_strMyRole == _T("의사")) m_cmbAction.AddString(_T("SAVE"));
	else if (m_strMyRole == _T("경찰")) m_cmbAction.AddString(_T("CHECK"));

	m_cmbAction.SetCurSel(0);
	InitPlayerList();
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
		strJson.Format("{\"op\": \"NIGHT_ACTION\", \"target\": \"%s\"}", (LPCSTR)asciiTarget);
		m_pSocket->SendJson(strJson);
	}

	CString log;
	log.Format(_T("[시스템] '%s'님에게 능력을 사용했습니다.\r\n"), (LPCTSTR)targetName);
	AppendChat(log);

	m_bActionSubmitted = true;
	m_btnConfirm.EnableWindow(FALSE);
	m_cmbAction.EnableWindow(FALSE);
	m_playerList.EnableWindow(FALSE);
}

// CNightDlg.cpp 의 OnClickedSend 함수 (보내기 버튼)

void CNightDlg::OnClickedSend()
{
	CString msg;
	m_chatInput.GetWindowText(msg);
	msg.Trim();
	if (msg.IsEmpty()) return;

	// [수정] 밤에는 마피아만 채팅 가능
	if (m_strMyRole != _T("마피아"))
	{
		AppendChat(_T("[시스템] 밤에는 대화할 수 없습니다. (마피아 제외)\r\n"));
		m_chatInput.SetWindowText(_T(""));
		return;
	}

	// 1. 서버로 전송
	if (m_pSocket) {
		CStringA strJson;
		CT2A asciiMsg(msg, CP_UTF8);
		strJson.Format("{\"op\": \"MAFIA_CHAT\", \"text\": \"%s\"}", (LPCSTR)asciiMsg);
		m_pSocket->SendJson(strJson);
	}

	// 2. ★ [수정] 내 화면에 즉시 표시 (이걸 살려야 내가 쓴 글이 바로 보입니다)
	CString line;
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

// CNightDlg.cpp 의 OnReceiveMsg 함수 (서버 메시지 수신)

LRESULT CNightDlg::OnReceiveMsg(WPARAM wParam, LPARAM lParam)
{
	CStringA* pJsonA = (CStringA*)wParam;
	if (!pJsonA) return 0;
	CStringA strJson = *pJsonA;
	delete pJsonA;

	// 1. 밤 결과 (사망자 발생)
	if (strJson.Find("\"op\": \"NIGHT_RESULT\"") != -1)
	{
		CString strVictim = _T("");
		int nVic = strJson.Find("\"victim\": \"");
		if (nVic != -1) {
			CStringA sVal = strJson.Mid(nVic + 11);
			int nEnd = sVal.Find('\"');
			if (nEnd != -1) sVal = sVal.Left(nEnd);
			strVictim = CString(CA2T(sVal));
		}
		bool bSaved = (strJson.Find("\"saved\": true") != -1);

		if (!strVictim.IsEmpty() && !bSaved)
		{
			// 리스트 갱신
			for (auto& p : m_players) {
				if (p.strUID == strVictim) { p.alive = false; break; }
			}
			InitPlayerList();

			// 부모 데이터 동기화
			CMafia43Dlg* pMain = dynamic_cast<CMafia43Dlg*>(GetParent());
			if (pMain) {
				for (auto& roomPlayer : pMain->m_vecRoomPlayers) {
					if (roomPlayer.strUID == strVictim) {
						roomPlayer.bIsAlive = false; break;
					}
				}
			}

			if (strVictim == m_strMyUID) {
				KillTimer(1);
				AfxMessageBox(_T("마피아에게 습격당해 사망했습니다."));
				EndDialog(IDABORT);
				return 0;
			}
			else {
				CString msg;
				msg.Format(_T("[속보] 플레이어(%s)가 습격당했습니다.\r\n"), (LPCTSTR)strVictim);
				AppendChat(msg);
			}
		}
		else {
			AppendChat(_T("[속보] 밤 동안 아무도 죽지 않았습니다.\r\n"));
		}
	}
	// 2. 경찰 조사 결과
	else if (strJson.Find("\"op\": \"COP_RESULT\"") != -1)
	{
		bool bIsMafia = (strJson.Find("\"is_mafia\": true") != -1);
		CString msg;
		if (bIsMafia) msg = _T("조사 결과: 해당 플레이어는 [마피아] 입니다.");
		else msg = _T("조사 결과: 해당 플레이어는 [마피아]가 아닙니다.");
		AfxMessageBox(msg);
		AppendChat(msg + _T("\r\n"));
	}
	// 3. ★★★ [문제 2번 해결] 채팅 수신 (MAFIA_CHAT) ★★★
	else if (strJson.Find("\"op\": \"CHAT\"") != -1 || strJson.Find("\"op\": \"MAFIA_CHAT\"") != -1)
	{
		// 1) 텍스트 파싱
		CString text = _T("");
		int nText = strJson.Find("\"text\": \"");
		if (nText != -1) {
			CStringA sText = strJson.Mid(nText + 9); // "text": " 길이
			int nEnd = sText.Find('\"');
			if (nEnd != -1) sText = sText.Left(nEnd);
			text = CString(CA2T(sText, CP_UTF8));
		}

		// 2) 보낸 사람 이름 파싱 (서버 코드는 "from"을 보냄)
		CString sender = _T("Unknown");
		int nName = strJson.Find("\"from\": \""); // 서버 코드 기준
		if (nName != -1) {
			CStringA sName = strJson.Mid(nName + 9); // "from": " 길이
			int nEndName = sName.Find('\"');
			if (nEndName != -1) sName = sName.Left(nEndName);
			sender = CString(CA2T(sName, CP_UTF8));
		}
		else {
			// 혹시 모를 호환성을 위해 from_name도 체크
			nName = strJson.Find("\"from_name\": \"");
			if (nName != -1) {
				CStringA sName = strJson.Mid(nName + 14);
				int nEndName = sName.Find('\"');
				if (nEndName != -1) sName = sName.Left(nEndName);
				sender = CString(CA2T(sName, CP_UTF8));
			}
		}

		// 내 메시지가 다시 돌아온 경우(서버 에코), 내가 이미 OnClickedSend에서 띄웠으므로 무시
		// (단, 닉네임이 같아야 함. 닉네임이 다르면 보여줌)
		/*
		if (sender == m_strMyNickname) {
			return 0;
		}
		*/

			// 내가 보낸 메시지가 다시 돌아온 경우(서버 정책에 따라 다름) 중복 표시 방지 로직을 넣을 수도 있으나,
			// 일단 다 표시하는 것이 안전함.
			CString msg;
			msg.Format(_T("%s: %s\r\n"), (LPCTSTR)sender, (LPCTSTR)CString(CA2T(sText, CP_UTF8)));
			AppendChat(msg);
		}
	}ㅇㅇ
	// 4. 게임 종료
	else if (strJson.Find("\"op\": \"GAME_END\"") != -1)
	{
		KillTimer(1);
		AfxMessageBox(_T("게임이 종료되었습니다!"));
		EndDialog(IDABORT);
		return 0;
	}
	// 5. 다음 페이즈(낮) 이동
	else if (strJson.Find("\"phase\": \"DAY\"") != -1)
	{
		RequestPhaseChange(false);
	}

	return 0;
}

void CNightDlg::RequestPhaseChange(bool bNotifyServer)
{
	if (m_bNextPhaseRequested) return;

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