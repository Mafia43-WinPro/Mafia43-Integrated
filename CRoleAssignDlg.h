#pragma once
#include "afxdialogex.h"
#include "Resource.h"

// CRoleAssignDlg 대화 상자

class CRoleAssignDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CRoleAssignDlg)

public:
	CRoleAssignDlg(CWnd* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~CRoleAssignDlg();

	// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ROLE_ASSIGN };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()
public:
	CStatic m_picRole;
	virtual BOOL OnInitDialog();
	CStatic m_staticRoleName;
	afx_msg void OnClickedButtonRoleConfirm();
	afx_msg void OnStnClickedPicRole();

	// ▼▼▼ 여기에 '표시할 직업명'을 전달받을 변수를 추가합니다 ▼▼▼
	CString m_strRoleToShow;
};