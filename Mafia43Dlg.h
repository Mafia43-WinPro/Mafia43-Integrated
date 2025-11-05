// Mafia43Dlg.h: 헤더 파일
//
#pragma once
#include "CClientSocket.h" // 1. CClientSocket 헤더 추가
#include "Resource.h"
// ▼▼▼ CClientSocket에서 보낼 사용자 정의 메시지 ID ▼▼▼
#define WM_USER_CONNECT_SUCCESS (WM_USER + 100) // 접속 성공
#define WM_USER_CONNECT_FAIL    (WM_USER + 101) // 접속 실패
#define WM_USER_RECV_MSG        (WM_USER + 102) // 메시지 수신
#define WM_USER_SERVER_CLOSE    (WM_USER + 103) // 서버 끊김
#define WM_USER_GAME_START      (WM_USER + 104) // 게임 시작 신호


// CMafia43Dlg 대화 상자
class CMafia43Dlg : public CDialogEx
{
	// 생성입니다.
public:
	CMafia43Dlg(CWnd* pParent = nullptr);	// 표준 생성자입니다.

	// --- 2. 소켓 및 상태 변수 추가 ---
	CClientSocket m_Socket;   // 비동기 소켓 멤버
	CString m_strMyUID;     // 서버가 발급한 내 ID
	CString m_strRoomID;    // 내가 현재 입장한 방 ID
	CString m_strMyRole;    // 내 직업

	// --- 3. JSON 파싱 및 처리 함수 선언 ---
	void ProcessServerMessage(CStringA strJsonA);
	void ParseHello(const CStringA& strJsonA);
	void ParseRoomList(const CStringA& strJsonA);
	void ParseRoomState(const CStringA& strJsonA);
	void ParseRole(const CStringA& strJsonA);

	// CStringA (UTF-8) -> CString (TCHAR) 변환 헬퍼
	CStringA CStr_to_CStrA(const CString& strT);
	CString CStrA_to_CStr(const CStringA& strA);


	// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_MAFIA43_DIALOG };
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
	// --- 4. 컨트롤 변수 (UI 편집기에서 설정한 ID와 일치해야 함) ---

	// '방 목록' 리스트 (기존 IDC_LIST_PLAYERS)
	CListCtrl m_listRooms;

	// '방 안의 플레이어 목록' 리스트 (새로 추가한 IDC_LIST_PLAYERS_IN_ROOM)
	CListCtrl m_listPlayersInRoom;

	// '방 만들기'에 사용할 '방 제목' (새로 추가한 IDC_EDIT_ROOM_TITLE)
	CString m_strRoomTitle;

	// '닉네임' (기존 IDC_EDIT_NICKNAME)
	CString m_strNickname;

	// '방 정보' 표시용 (기존 IDC_STATIC_ROOMCODE)
	CStatic m_staticRoomInfo;


	// --- 5. 버튼 핸들러 함수 ---

	// '서버 접속' 버튼 (기존 IDC_BUTTON_START)
	afx_msg void OnClickedButtonConnect();

	// '나가기' 버튼 (기존 IDC_BUTTON_EXIT)
	afx_msg void OnClickedButtonExit();

	// '방 만들기' 버튼 (새로 추가한 IDC_BTN_CREATE_ROOM)
	afx_msg void OnClickedButtonCreateRoom();

	// '방 입장' 버튼 (새로 추가한 IDC_BTN_JOIN_ROOM)
	afx_msg void OnClickedButtonJoinRoom();

	// '게임 시작' 버튼 (새로 추가한 IDC_BTN_START_GAME)
	afx_msg void OnClickedButtonStartGame();


	// --- 6. 사용자 정의 메시지 핸들러 선언 ---
	afx_msg LRESULT OnConnectSuccess(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnConnectFail(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnRecvMsg(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnServerClose(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnGameStart(WPARAM wParam, LPARAM lParam);

	// 타이머 핸들러
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnLvnItemchangedListPlayers(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnStnClickedPicBackground();
};