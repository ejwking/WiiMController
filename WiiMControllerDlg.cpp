
// WiiMControllerDlg.cpp : implementation file
//


/* NOTES

* CString advantages
Integrates perfectly with MFC controls (SetItemText, MessageBox, etc.)
Automatic conversion to/from LPCTSTR
Built‑in formatting (Format)
Built‑in trimming, searching, replacing
Handles Unicode correctly without extra work

* std::string advantages
Standard C++ (portable)
Works well with STL algorithms
Often faster for pure C++ logic
No dependency on MFC

* The hybrid approach (best for MFC apps)
Most MFC applications naturally end up using both:
UI layer → CString
Internal data → std::string

* Example of clean conversion between CString and std::string
CString to std::string  
std::string s = CT2A(cstr);

std::string to CString
CString cstr = CA2T(s.c_str());

These macros handle Unicode/ANSI correctly.

*/




#include "pch.h"
#include "framework.h"
#include "WiiMController.h"
#include "WiiMControllerDlg.h"
#include "afxdialogex.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define REG_SECTION	_T("WiimCntrlWnd")

CWiiMControllerDlg::CWiiMControllerDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_WIIMCONTROLLER_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_ItemCount = 0;
}

void CWiiMControllerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_STREAMURL, m_ListStream);
}

BEGIN_MESSAGE_MAP(CWiiMControllerDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_TEST,&CWiiMControllerDlg::OnBnClickedBtnTest)
	ON_BN_CLICKED(IDC_BTN_INIT,&CWiiMControllerDlg::OnBnClickedBtnInit)
	ON_NOTIFY(LVN_ITEMCHANGED,IDC_LIST_STREAMURL,&CWiiMControllerDlg::OnLvnItemchangedListStreamurl)
	ON_BN_CLICKED(IDC_BTN_LOAD_FILE,&CWiiMControllerDlg::OnBnClickedBtnLoadFile)
	ON_BN_CLICKED(IDC_BTN_BROWSE,&CWiiMControllerDlg::OnBnClickedBtnBrowse)
END_MESSAGE_MAP()


// CWiiMControllerDlg message handlers

BOOL CWiiMControllerDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	AddColumnsToStreamUrlList();

	m_StreamsFilepath = AfxGetApp()->GetProfileString(REG_SECTION, _T("StreamsFilepath"), _T(""));
	GetDlgItem(IDC_EDIT_STREAMSFILE)->SetWindowText(m_StreamsFilepath);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CWiiMControllerDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CWiiMControllerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CWiiMControllerDlg::AddColumnsToStreamUrlList()
{
	m_ListStream.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	// add 2 columns, 1) stream name, 2) stream URL
	m_ListStream.InsertColumn(0, _T("Stream Name"), LVCFMT_LEFT, 150);
	m_ListStream.InsertColumn(1, _T("Stream URL"), LVCFMT_LEFT, 2000);
}

void CWiiMControllerDlg::AddListRow(int index)
{
	const LISTDATA &d = m_ListData[index];
	int item = m_ListStream.InsertItem(index, CA2T(d.name.c_str()));
	m_ListStream.SetItemText(item, 1, CA2T(d.url.c_str()));
}

bool CWiiMControllerDlg::LoadStreamUrlsFromFile(const CString& filePath)
{
	CStdioFile file;
	if (!file.Open(filePath, CFile::modeRead | CFile::typeText))
		return false;

	m_ListStream.DeleteAllItems(); // Clear existing items before loading new ones
	CString line;
	m_ItemCount = 0;
	while (file.ReadString(line)){
		if(!line.IsEmpty()){	// skip empty lines
			if(line[0] != '#'){	// skip lines starting with # (comments)
				if(m_ItemCount < LIST_MAX){
					CString name = line;
					// Read URL line
					if (!file.ReadString(line))
						break;  // malformed file
					CString url = line;
					name.Trim(); // Remove leading/trailing whitespace
					url.Trim();

					// to do, 
					// test that url looks like a URL, starts with http:// or https://.
					// also test theres not an empty line between name and url.

					// Store in your struct array
					m_ListData[m_ItemCount].name = CT2A(name);
					m_ListData[m_ItemCount].url  = CT2A(url);
					m_ItemCount++;
				}
				else{
					AfxMessageBox(_T("Reached maximum list capacity. Some entries may not be loaded."));
					break;
				}
			}
		}
	}
	file.Close();
	return true;
}

void CWiiMControllerDlg::OnBnClickedBtnTest()
{

//	m_httpClient.ToggleMute();
	// TODO: Add your control notification handler code here
}

void CWiiMControllerDlg::OnBnClickedBtnInit()
{
//	m_httpClient.SetBaseUrl("192.168.0.228");
}

void CWiiMControllerDlg::OnLvnItemchangedListStreamurl(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: Add your control notification handler code here
	*pResult = 0;

	TRACE("\n\n %d", pNMLV->iItem); // index of the changed item

	m_httpClient.SetBaseUrl("192.168.0.228");

	m_httpClient.PlayUrl(m_ListData[pNMLV->iItem].url);
}

void CWiiMControllerDlg::OnBnClickedBtnLoadFile()
{
	if(LoadStreamUrlsFromFile(m_StreamsFilepath))
		for(int i=0; i<m_ItemCount; i++)
			AddListRow(i);
}

void CWiiMControllerDlg::OnBnClickedBtnBrowse()
{
	CFileDialog FileDlg(TRUE, _T("txt"), nullptr, OFN_FILEMUSTEXIST, _T("Text Files (*.txt)|*.txt|All Files (*.*)|*.*||"));
	if(FileDlg.DoModal() == IDOK){
		m_StreamsFilepath = FileDlg.GetPathName();
		GetDlgItem(IDC_EDIT_STREAMSFILE)->SetWindowText(m_StreamsFilepath);
		AfxGetApp()->WriteProfileString(REG_SECTION, _T("StreamsFilepath"), m_StreamsFilepath);

	//	UpdateData(0); // to dlg
	}
}

// need a sort button as well.