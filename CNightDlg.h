#pragma once
#include "afxdialogex.h"
#include "CClientSocket.h" // 서버 통신용 헤더
#include <vector>

// 플레이어 정보 구조체
struct PlayerInfo {
	int id;
	CString name;
	bool alive;
};

// CNightDlg 대화 상자
class CNightDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CNightDlg)

public:
	//CNightDlg(CWnd* pParent = nullptr);   // 표준 생성자입니다.
	CNightDlg(const std::vector<PlayerInfo>& players, CWnd* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~CNightDlg();
	void SetSocket(CClientSocket* pSocket) { m_pSocket = pSocket; }
	bool m_bActionSubmitted;

	// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_NIGHT_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	// 구현입니다.
protected:
	HICON m_hIcon;

	virtual BOOL OnInitDialog();
	virtual void OnOK(); // [중요] 낮 화면으로 넘어가기 위한 함수
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();

	// 팀원분이 작성한 핸들러들
	afx_msg void OnBnClickedConfirm();
	afx_msg void OnClickedSend();
	afx_msg void OnClickedSkip();
	afx_msg void OnItemchangedPlayerList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnEnChangeReChatview();

	DECLARE_MESSAGE_MAP()

public:
	// --- [통신용 멤버 변수] ---
	CClientSocket* m_pSocket;  // 서버 소켓
	CString m_strMyNickname;
	CString m_strMyRole;

	// --- [UI 컨트롤 변수] ---
	// (주의: 리소스 뷰에서 이 ID들을 가진 컨트롤을 만들어야 합니다!)
	CListCtrl m_playerList;      // IDC_LIST_PLAYERS
	CComboBox m_cmbAction;       // IDC_CMB_ACTION
	CButton m_btnConfirm;        // IDC_BTN_CONFIRM
	CStatic m_lblRole;           // IDC_LBL_ROLE
	CStatic m_lblTimer;          // IDC_LBL_TIMER
	CStatic m_lblPreview;        // IDC_LBL_PREVIEW
	CRichEditCtrl m_chatView;    // IDC_RE_CHATVIEW (채팅 내용)
	CEdit m_chatInput;           // IDC_EDT_CHAT (채팅 입력)
	CButton m_btnSend;           // IDC_BTN_SEND

	// --- [데이터 변수] ---
	std::vector<PlayerInfo> m_players;
	int m_selectedTargetId = 0;
	int m_timeLeftSec = 0;

	// 헬퍼 함수
	void InitPlayerList();
	void AppendChat(CString strMsg); // 채팅창 출력 헬퍼
	afx_msg void OnBnClickedButton2();
};
