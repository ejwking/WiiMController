
// WiiMControllerDlg.h : header file
//

#pragma once

#include "Network.h"

#define EQ_MAX 150
#define LIST_MAX 256

struct STREAMINFO
{
	CString category, name, url;
};

// CWiiMControllerDlg dialog
class CWiiMControllerDlg : public CDialog
{
// Construction
public:
	CWiiMControllerDlg(CWnd* pParent = nullptr);	// standard constructor

	int m_DeviceAvailable, m_InitUI;
	CWiimHttpClient m_httpClient;
	CListCtrl m_ListStream, m_ListEQ;

	CString m_EQname[EQ_MAX]; // todo use std::vector<CString> instead of fixed-size array
	STREAMINFO m_StreamInfo[LIST_MAX]; // todo use std::vector<STREAMDATA> instead of fixed-size array

	int m_NumStream, m_NumEQ;
	CString m_StreamsFilepath, m_LastStatus;

	bool LoadStreamUrlsFromFile(const CString& filePath);
	void LoadStreamUrlList();
	void UpdatePlayerStatusString();
	void LoadEqualiserPresetsList(char *str);
	void GetInfoFromDevice();
	void UpdateStatusEditBox();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_WIIMCONTROLLER_DIALOG };
#endif

// Implementation
protected:
	HICON m_hIcon;

	DECLARE_MESSAGE_MAP()

	// Generated message map functions
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();

public:
	
	afx_msg void OnLvnItemchangedListEq(NMHDR *pNMHDR,LRESULT *pResult);
	afx_msg void OnLvnItemchangedListStreamurl(NMHDR *pNMHDR,LRESULT *pResult);
	afx_msg void OnBnClickedBtnLoadFile();
	afx_msg void OnBnClickedBtnBrowse();
	afx_msg void OnBnClickedBtnToggleEq();
	afx_msg void OnBnClickedBtnRefreshStats();
	afx_msg void OnBnClickedBtnVolUp();
	afx_msg void OnBnClickedBtnVolDown();
	afx_msg void OnBnClickedBtnMute();
	afx_msg void OnBnClickedBtnTogglePlay();
};
