// CRoleAssignDlg.cpp: 구현 파일
//

#include "pch.h"
#include "Mafia43.h"
#include "afxdialogex.h"
#include "CRoleAssignDlg.h"


// CRoleAssignDlg 대화 상자

IMPLEMENT_DYNAMIC(CRoleAssignDlg, CDialogEx)

CRoleAssignDlg::CRoleAssignDlg(CWnd* pParent /*=nullptr*/)
    : CDialogEx(IDD_ROLE_ASSIGN, pParent)
{
    // m_strRoleToShow는 생성자에서 초기화할 필요 없습니다.
    // 부모(CMafia43Dlg)가 값을 직접 넣어줄 것이기 때문입니다.
}

CRoleAssignDlg::~CRoleAssignDlg()
{
}

void CRoleAssignDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_PIC_ROLE, m_picRole);
    DDX_Control(pDX, IDC_STATIC_ROLENAME, m_staticRoleName);
}


BEGIN_MESSAGE_MAP(CRoleAssignDlg, CDialogEx)
    ON_BN_CLICKED(IDC_BUTTON_ROLE_CONFIRM, &CRoleAssignDlg::OnClickedButtonRoleConfirm)
    ON_STN_CLICKED(IDC_PIC_ROLE, &CRoleAssignDlg::OnStnClickedPicRole)
END_MESSAGE_MAP()


// CRoleAssignDlg 메시지 처리기


BOOL CRoleAssignDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // TODO: 여기에 추가 초기화 작업을 추가합니다.

    // 1. 역할 텍스트를 전달받은 값(m_strRoleToShow)으로 설정합니다.
    CString strText;
    strText.Format(_T("당신의 역할은 [%s] 입니다."), m_strRoleToShow);
    m_staticRoleName.SetWindowText(strText);

    // 2. 역할에 맞는 비트맵 리소스 ID를 찾습니다.
    int nBitmapID = IDB_ROLE_CITIZEN; // 기본값 (시민)
    if (m_strRoleToShow == _T("마피아"))
    {
        nBitmapID = IDB_ROLE_MAFIA;
    }
    else if (m_strRoleToShow == _T("의사"))
    {
        nBitmapID = IDB_ROLE_DOCTOR; // (Resource.h에 1012로 정의됨)
    }
    else if (m_strRoleToShow == _T("경찰"))
    {
        nBitmapID = IDB_ROLE_POLICE; // (Resource.h에 1013으로 정의됨)
    }
    // (시민은 IDB_ROLE_CITIZEN (1014) 사용)


    // 3. 해당 ID의 비트맵 리소스를 로드합니다.
    HBITMAP hBitmap = (HBITMAP)LoadImage(AfxGetInstanceHandle(),
        MAKEINTRESOURCE(nBitmapID), // 찾은 ID 사용
        IMAGE_BITMAP,
        0, 0, // 이미지 원본 크기 사용
        LR_DEFAULTCOLOR);

    // 4. Picture Control에 비트맵을 설정합니다.
    if (hBitmap)
    {
        m_picRole.SetBitmap(hBitmap);
    }

    // 5. 창 크기를 강제로 조절합니다.
    SetWindowPos(NULL, 0, 0, 450, 600, SWP_NOMOVE | SWP_NOZORDER);

    // 6. (선택) 텍스트와 버튼을 그림 아래로 강제 이동 (필요한 경우 주석 해제)
    /*
    CWnd* pButton = GetDlgItem(IDC_BUTTON_ROLE_CONFIRM);
    if (pButton)
    {
        m_staticRoleName.MoveWindow(20, 540, 410, 20); // 예시 좌표
        pButton->MoveWindow(190, 570, 70, 20);      // 예시 좌표

        // 버튼 맨 앞으로 가져오기
        pButton->SetWindowPos(&wndTop, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }
    */

    return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}


void CRoleAssignDlg::OnClickedButtonRoleConfirm()
{
    // TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
    OnOK();
}


void CRoleAssignDlg::OnStnClickedPicRole()
{
    // TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
}