// CNightDlg.h
// ... (기존 코드) ...

class CNightDlg : public CDialogEx
{
	// ... (기존 코드) ...

	// 구현입니다.
protected:
	DECLARE_MESSAGE_MAP()

public:
	// --- 부모로부터 전달받는 멤버 변수 ---
	CClientSocket* m_pSocket;
	CString m_strMyNickname;
	CString m_strMyRole;

	// --- [추가] 메시지 핸들러 및 가상 함수 ---
	virtual BOOL OnInitDialog(); // [추가]
	virtual void OnOK();           // [추가]
};