
// nightDlg.h: 헤더 파일
//

#pragma once
#include <vector>

struct PlayerInfo {
	int id;
	CString name;
	bool alive;
};

enum class Role { Mafia, Doctor, Cop, Civilian };

// CnightDlg 대화 상자
class CnightDlg : public CDialogEx
{
// 생성입니다.
public:
	CnightDlg(CWnd* pParent = nullptr);	// 표준 생성자입니다.

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_NIGHT_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.


// 구현입니다.
protected:
	HICON m_hIcon;

	// 생성된 메시지 맵 함수
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	CListCtrl m_playerList;
	CComboBox m_cmbAction;
	CButton m_btnConfirm;
	CStatic m_lblRole;
	CStatic m_lblTimer;
	CStatic m_lblPreview;
	CRichEditCtrl m_chatView;
	CEdit m_chatInput;
	CButton m_btnSend;
	std::vector<PlayerInfo> m_players;
	afx_msg void OnBnClickedConfirm();
	afx_msg void OnClickedSend();
	afx_msg void OnItemchangedPlayerList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnEnChangeReChatview();
	afx_msg void OnClickedSkip();
};
