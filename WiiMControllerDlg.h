
// WiiMControllerDlg.h : header file
//

#pragma once

#include "Network.h"


// CWiiMControllerDlg dialog
class CWiiMControllerDlg : public CDialog
{
// Construction
public:
	CWiiMControllerDlg(CWnd* pParent = nullptr);	// standard constructor

	
	CWiimHttpClient m_httpClient; // MAYBE DONT OPEN CURL UNTIL FIRST REQUEST IS SENT, ALSO NO POINT UNTIL DISCOVERY IS DONE. LEAVE IT LIKE THIS FOR NOW.


// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_WIIMCONTROLLER_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	DECLARE_MESSAGE_MAP()

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnBnClickedBtnTest();
public:
	afx_msg void OnBnClickedBtnInit();
};
