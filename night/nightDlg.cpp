
// nightDlg.cpp: 구현 파일
//

#include "pch.h"
#include "framework.h"
#include "night.h"
#include "nightDlg.h"
#include "afxdialogex.h"

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


// CnightDlg 대화 상자



CnightDlg::CnightDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_NIGHT_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CnightDlg::DoDataExchange(CDataExchange* pDX)
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

BEGIN_MESSAGE_MAP(CnightDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_CONFIRM, &CnightDlg::OnBnClickedConfirm)
	ON_BN_CLICKED(IDC_BTN_SEND, &CnightDlg::OnClickedSend)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_PLAYERS, &CnightDlg::OnItemchangedPlayerList)
	ON_WM_TIMER()
	ON_EN_CHANGE(IDC_RE_CHATVIEW, &CnightDlg::OnEnChangeReChatview)
	ON_BN_CLICKED(IDC_BTN_SKIP, &CnightDlg::OnClickedSkip)
END_MESSAGE_MAP()


// CnightDlg 메시지 처리기

BOOL CnightDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 시스템 메뉴에 "정보..." 메뉴 항목을 추가합니다.

	// IDM_ABOUTBOX는 시스템 명령 범위에 있어야 합니다.
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

	// 이 대화 상자의 아이콘을 설정합니다.  응용 프로그램의 주 창이 대화 상자가 아닐 경우에는
	//  프레임워크가 이 작업을 자동으로 수행합니다.
	SetIcon(m_hIcon, TRUE);			// 큰 아이콘을 설정합니다.
	SetIcon(m_hIcon, FALSE);		// 작은 아이콘을 설정합니다.

	// TODO: 여기에 추가 초기화 작업을 추가합니다.
	m_playerList.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	m_playerList.InsertColumn(0, _T("이름"), LVCFMT_LEFT, 140);

	m_lblRole.SetWindowText(L"역할: (대기중)");
	m_lblTimer.SetWindowText(L"남은 시간: 00:00");
	m_lblPreview.SetWindowText(L"선택: 없음 → 없음");

	m_chatView.SetReadOnly(TRUE);

	m_cmbAction.ResetContent();
	m_cmbAction.AddString(L"NONE");
	m_cmbAction.SetCurSel(0);

	m_players = {
		{1, L"홍길동", true},
		{2, L"이순신", false},
		{3, L"유관순", true},
		{4, L"강감찬", true}
	};

	InitPlayerList();   // ✅ 반드시 호출해야 화면에 보임

	m_timeLeftSec = 30; // 예시
	SetTimer(1, 1000, nullptr);






	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

void CnightDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

// 대화 상자에 최소화 단추를 추가할 경우 아이콘을 그리려면
//  아래 코드가 필요합니다.  문서/뷰 모델을 사용하는 MFC 애플리케이션의 경우에는
//  프레임워크에서 이 작업을 자동으로 수행합니다.

void CnightDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 그리기를 위한 디바이스 컨텍스트입니다.

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 클라이언트 사각형에서 아이콘을 가운데에 맞춥니다.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 아이콘을 그립니다.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// 사용자가 최소화된 창을 끄는 동안에 커서가 표시되도록 시스템에서
//  이 함수를 호출합니다.
HCURSOR CnightDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CnightDlg::OnBnClickedConfirm()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	int sel = m_cmbAction.GetCurSel();
	CString action;
	if (sel >= 0)
		m_cmbAction.GetLBText(sel, action);
	else
		action = L"NONE";

	// ② 대상 선택
	int item = m_playerList.GetNextItem(-1, LVNI_SELECTED);
	int targetId = 0;
	if (item != -1)
		targetId = (int)m_playerList.GetItemData(item);

	// ③ 유효성 체크
	if (action != L"NONE" && targetId == 0) {
		AfxMessageBox(L"대상을 선택하세요!");
		return;
	}

	// ④ 로그 출력 (또는 서버 전송)
	CString log;
	log.Format(L"행동 제출 → [%s] 대상 ID: %d\r\n", action, targetId);
	m_chatView.ReplaceSel(log);
	
}

void CnightDlg::OnClickedSend()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	CString msg;
	m_chatInput.GetWindowTextW(msg);
	msg.Trim();
	if (msg.IsEmpty()) return;

	CString line;
	line.Format(L"[나] %s\r\n", msg);
	m_chatView.ReplaceSel(line);

	m_chatInput.SetWindowTextW(L""); // 입력창 비우기
}

void CnightDlg::OnItemchangedPlayerList(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	LPNMLISTVIEW p = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	if ((p->uChanged & LVIF_STATE) && (p->uNewState & LVIS_SELECTED)) {
		int idx = p->iItem;
		if (idx >= 0) {
			m_selectedTargetId = (int)m_playerList.GetItemData(idx);

			// 미리보기 라벨 갱신
			CString name = m_playerList.GetItemText(idx, 0);
			CString preview;
			preview.Format(L"선택 대상: %s", name);
			m_lblPreview.SetWindowTextW(preview);
		}
	}

	*pResult = 0;
}

void CnightDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == 1 && m_timeLeftSec > 0)
	{
		--m_timeLeftSec;

		CString s;
		s.Format(L"남은 시간: %02d:%02d", m_timeLeftSec / 60, m_timeLeftSec % 60);
		m_lblTimer.SetWindowTextW(s);

		if (m_timeLeftSec == 0)
		{
			KillTimer(1);
			CString log;
			log.Format(L"시간 만료 → 자동 NONE 행동\r\n");
			m_chatView.ReplaceSel(log);
		}
	}

	CDialogEx::OnTimer(nIDEvent);
}


void CnightDlg::OnEnChangeReChatview()
{
	// TODO:  RICHEDIT 컨트롤인 경우, 이 컨트롤은
	// CDialogEx::OnInitDialog() 함수를 재지정 
	//하고 마스크에 OR 연산하여 설정된 ENM_CHANGE 플래그를 지정하여 CRichEditCtrl().SetEventMask()를 호출하지 않으면
	// ENM_CHANGE가 있으면 마스크에 ORed를 플래그합니다.

	// TODO:  여기에 컨트롤 알림 처리기 코드를 추가합니다.
}

void CnightDlg::OnClickedSkip()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	CString log;
	log.Format(L"행동 건너뛰기 (NONE)\r\n");
	m_chatView.ReplaceSel(log);
}

void CnightDlg::InitPlayerList()
{
	m_playerList.DeleteAllItems();

	for (const auto& p : m_players)
	{
		if (!p.alive)
			continue; // ✨ 사망자는 표시 안 함

		int row = m_playerList.InsertItem(m_playerList.GetItemCount(), p.name);
		m_playerList.SetItemData(row, p.id);
	}
}
