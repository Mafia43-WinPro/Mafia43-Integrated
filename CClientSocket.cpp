// ClientSocket.cpp
#include "pch.h"
#include "CClientSocket.h"
#include "Mafia43Dlg.h" // 메인 다이얼로그 헤더

CClientSocket::CClientSocket()
{
    m_pDlg = nullptr;
}

CClientSocket::~CClientSocket()
{
}

// 1. 서버 접속 성공 시
void CClientSocket::OnConnect(int nErrorCode)
{
    if (nErrorCode == 0)
    {
        // 접속 성공 메시지를 메인 다이얼로그에 알림
        if (m_pDlg)
        {
            // 메인 스레드로 안전하게 메시지 전송
            m_pDlg->PostMessage(WM_USER_CONNECT_SUCCESS);
        }
    }
    else
    {
        if (m_pDlg)
        {
            m_pDlg->PostMessage(WM_USER_CONNECT_FAIL);
        }
    }
    CAsyncSocket::OnConnect(nErrorCode);
}

// 2. 서버가 데이터 보냈을 때 (가장 중요)
void CClientSocket::OnReceive(int nErrorCode)
{
	if (nErrorCode == 0)
	{
		// 1. 데이터를 받을 수 있는 최대 크기 확인
		DWORD dwDataSize = 0;
		if (!IOCtl(FIONREAD, &dwDataSize) || dwDataSize == 0)
		{
			CAsyncSocket::OnReceive(nErrorCode);
			return;
		}

		// 2. CStringA 버퍼를 준비 (이 방식이 char*보다 안전함)
		//    (데이터가 클 경우 dwDataSize 만큼만 읽어옴)
		CStringA strRecvData;
		int nBytesRecv = Receive(strRecvData.GetBuffer(dwDataSize), dwDataSize);
		strRecvData.ReleaseBuffer(nBytesRecv);

		if (nBytesRecv > 0)
		{
			// 3. 수신된 데이터를 메인 버퍼에 추가
			m_strBuffer += strRecvData;

			// 4. 버퍼에서 \n (줄바꿈)을 기준으로 JSON 메시지를 분리
			int nPos = -1;
			while ((nPos = m_strBuffer.Find('\n')) != -1)
			{
				// 5. \n까지의 문자열(완전한 JSON)을 추출
				CStringA strJsonA = m_strBuffer.Left(nPos);

				// 6. 버퍼에서 추출한 메시지 삭제
				m_strBuffer = m_strBuffer.Mid(nPos + 1);

				// 7. 메인 다이얼로그의 처리 함수로 JSON 전달
				if (m_pDlg && !strJsonA.IsEmpty())
				{
					CStringA* pJsonPacket = new CStringA(strJsonA);
					m_pDlg->PostMessage(WM_USER_RECV_MSG, (WPARAM)pJsonPacket);
				}
			}
		}
	}

	CAsyncSocket::OnReceive(nErrorCode);
}

// 3. 서버 접속 끊겼을 때
void CClientSocket::OnClose(int nErrorCode)
{
    if (m_pDlg)
    {
        m_pDlg->PostMessage(WM_USER_SERVER_CLOSE);
    }
    CAsyncSocket::OnClose(nErrorCode);
}

// 4. 서버로 JSON 전송 (CStringA 사용, \n 추가)
void CClientSocket::SendJson(const CStringA& strJsonA)
{
    if (m_hSocket == INVALID_SOCKET) return;

    // JSON 문자열 끝에 \n을 붙여서 전송해야 서버가 인식함
    CStringA strMsg = strJsonA + "\n";
    Send(strMsg, strMsg.GetLength());
}