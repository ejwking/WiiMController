
// WiiMControllerDlg.h : header file
//

#pragma once

#include "Network.h"
#include <vector>

struct STREAMINFO
{
	CString category, name, url;
};

struct IPADDRESS
{
	BYTE Field0, Field1, Field2, Field3;
};

// CWiiMControllerDlg dialog
class CWiiMControllerDlg : public CDialog
{
// Construction
public:
	CWiiMControllerDlg(CWnd* pParent = nullptr);	// standard constructor

	int m_Initialised;

	CWiimHttpClient         m_httpClient;
	CIPAddressCtrl          m_IPCtrl;
	CListCtrl               m_ListStream, m_ListEQ;
	CFont                   m_HeaderFont;
	std::vector<CString>    m_EqPresetNames; 
	std::vector<STREAMINFO> m_StreamURLs;
	CString                 m_StreamsFilepath, m_LastStatus, m_WndErrorLog;
	IPADDRESS               m_IPAddress;
	int                     m_ListStream_SelectedIndex;

	int  LoadStreamUrlsFromFile(const CString& filePath);
	void LoadStreamUrlList();
	void UpdatePlayerStatusString();
	void LoadEqualiserPresetsList(char *str);
	void SelectListItems();
	void UpdateStatusEditBox();
	int  GetInfoFromDeviceAndPopulateUI();

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
	virtual BOOL DestroyWindow();
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
	afx_msg void OnIpnFieldchangedIpaddressWiim(NMHDR *pNMHDR,LRESULT *pResult);
};
