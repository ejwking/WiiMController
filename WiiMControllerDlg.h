
// WiiMControllerDlg.h : header file
//

#pragma once

#include "Network.h"

#define EQ_MAX 150

#define LIST_MAX 256
struct LISTDATA
{
	CString name;
	CString url;
};

// CWiiMControllerDlg dialog
class CWiiMControllerDlg : public CDialog
{
// Construction
public:
	CWiiMControllerDlg(CWnd* pParent = nullptr);	// standard constructor

	CWiimHttpClient m_httpClient;


// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_WIIMCONTROLLER_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

	void AddColumnsToStreamUrlList();
	void AddListRow(int index);
//	void AddStreamUrlToList(const std::string& url);

	bool LoadStreamUrlsFromFile(const CString& filePath);

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
	CListCtrl m_ListStream, m_ListEQ;

	CString m_EQname[EQ_MAX];

	LISTDATA m_ListData[LIST_MAX];
	int m_ItemCount;
	CString m_StreamsFilepath;

	afx_msg void OnLvnItemchangedListStreamurl(NMHDR *pNMHDR,LRESULT *pResult);
	afx_msg void OnBnClickedBtnLoadFile();
	afx_msg void OnBnClickedBtnBrowse();
	afx_msg void OnBnClickedBtnToggleEq();
};
