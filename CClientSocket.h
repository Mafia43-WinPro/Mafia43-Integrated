// ClientSocket.h
#pragma once
#include <afxsock.h> 

class CMafia43Dlg; // 전방 선언 (순환 참조 방지)

// ▼▼▼ 메인 다이얼로그로 보낼 사용자 메시지 ID ▼▼▼
// (Mafia43Dlg.h에도 똑같이 정의할 예정)
#define WM_USER_CONNECT_SUCCESS (WM_USER + 100) // 접속 성공
#define WM_USER_CONNECT_FAIL    (WM_USER + 101) // 접속 실패
#define WM_USER_RECV_MSG        (WM_USER + 102) // 메시지 수신
#define WM_USER_SERVER_CLOSE    (WM_USER + 103) // 서버 끊김

class CClientSocket : public CAsyncSocket
{
public:
    CClientSocket();
    virtual ~CClientSocket();

    CMafia43Dlg* m_pDlg; // 메인 다이얼로그를 가리킬 포인터
    CStringA m_strBuffer; // 데이터 수신용 임시 버퍼

    // MFC 마법사가 생성하는 올바른 함수 원형들
    virtual void OnConnect(int nErrorCode);
    virtual void OnReceive(int nErrorCode); // <--- void*가 아니라 int입니다.
    virtual void OnClose(int nErrorCode);

    // JSON 메시지를 서버로 보내는 헬퍼 함수
    void SendJson(const CStringA& strJsonA);
};