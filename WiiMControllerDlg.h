
// WiiMControllerDlg.h : header file
//

#pragma once

#include "Network.h"
#include <vector>
#include <string>


struct STREAMINFO
{
	std::string category, name, url;
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

	CWiimHttpClient m_httpClient;
	CIPAddressCtrl  m_IPCtrl;
	CFont           m_HeaderFont;
	CString         m_StreamsFilepath, m_LastStatus, m_UrlsErrorLog;
	IPADDRESS       m_IPAddress;
	
	// UI list data
	CListCtrl                m_ListStream, m_ListEQ;
	std::vector<std::string> m_EqPresetNames; 
	std::vector<STREAMINFO>  m_StreamURLs;
	int                      m_ListStream_SelectedIndex, m_ListEQ_SelectedIndex;

	bool LoadStreamUrlsFromUtf8File();
	void LoadStreamUrlList();
	void UpdatePlayerStatusString();
	void LoadEqualiserPresetsList(char *str);
	void SelectListItems();
	void UpdateStatusEditBox();
	int  GetInfoFromDeviceAndPopulateUI();
	void RestoreWindowPos();
	void ApplyCustomFont();
	void TrimString(std::string &s);
	int  HasBOM(std::string &line);

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
	afx_msg void OnBnClickedBtnRefreshList();
	afx_msg void OnBnClickedBtnOpen();
	afx_msg void OnBnClickedBtnToggleEq();
	afx_msg void OnBnClickedBtnRefreshStats();
	afx_msg void OnBnClickedBtnVolUp();
	afx_msg void OnBnClickedBtnVolDown();
	afx_msg void OnBnClickedBtnMute();
	afx_msg void OnBnClickedBtnTogglePlay();
	afx_msg void OnIpnFieldchangedIpaddressWiim(NMHDR *pNMHDR,LRESULT *pResult);
	afx_msg void OnDestroy();
};
