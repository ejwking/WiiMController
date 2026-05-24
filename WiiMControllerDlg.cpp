
// WiiMControllerDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "WiiMController.h"
#include "WiiMControllerDlg.h"
#include "afxdialogex.h"
#include "tools.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define REG_SECTION	_T("WiimCntrlWnd")
#define EQ_ON_TEXT	_T("equaliser is ON")
#define EQ_OFF_TEXT	_T("equaliser is OFF")

CWiiMControllerDlg::CWiiMControllerDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_WIIMCONTROLLER_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_Initialised = 0;
	m_ListStream_SelectedIndex = -1;
	//m_UrlsErrorLog = _T(""); unnecessary as CString default constructor already initializes to empty string.
}

void CWiiMControllerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX,IDC_LIST_STREAMURL,m_ListStream);
	DDX_Control(pDX,IDC_LIST_EQ,m_ListEQ);
	DDX_Control(pDX,IDC_IPADDRESS_WIIM,m_IPCtrl);
}

BEGIN_MESSAGE_MAP(CWiiMControllerDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	
	ON_NOTIFY(LVN_ITEMCHANGED,IDC_LIST_STREAMURL,&CWiiMControllerDlg::OnLvnItemchangedListStreamurl)
	ON_NOTIFY(LVN_ITEMCHANGED,IDC_LIST_EQ,&CWiiMControllerDlg::OnLvnItemchangedListEq)
	ON_NOTIFY(IPN_FIELDCHANGED,IDC_IPADDRESS_WIIM,&CWiiMControllerDlg::OnIpnFieldchangedIpaddressWiim)

	ON_BN_CLICKED(IDC_BTN_LOAD_FILE,&CWiiMControllerDlg::OnBnClickedBtnLoadFile)
	ON_BN_CLICKED(IDC_BTN_BROWSE,&CWiiMControllerDlg::OnBnClickedBtnBrowse)
	ON_BN_CLICKED(IDC_BTN_TOGGLE_EQ,&CWiiMControllerDlg::OnBnClickedBtnToggleEq)
	ON_BN_CLICKED(IDC_BTN_REFRESH_STATS,&CWiiMControllerDlg::OnBnClickedBtnRefreshStats)
	ON_BN_CLICKED(IDC_BTN_VOL_UP,&CWiiMControllerDlg::OnBnClickedBtnVolUp)
	ON_BN_CLICKED(IDC_BTN_VOL_DOWN,&CWiiMControllerDlg::OnBnClickedBtnVolDown)
	ON_BN_CLICKED(IDC_BTN_MUTE,&CWiiMControllerDlg::OnBnClickedBtnMute)
	ON_BN_CLICKED(IDC_BTN_TOGGLE_PLAY,&CWiiMControllerDlg::OnBnClickedBtnTogglePlay)
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
	SetWindowText(_T("WiiM Controller - v1.0"));

	m_IPAddress.Field0 = AfxGetApp()->GetProfileInt(REG_SECTION, _T("IP_Field0"), 192);
	m_IPAddress.Field1 = AfxGetApp()->GetProfileInt(REG_SECTION, _T("IP_Field1"), 168);
	m_IPAddress.Field2 = AfxGetApp()->GetProfileInt(REG_SECTION, _T("IP_Field2"), 0);
	m_IPAddress.Field3 = AfxGetApp()->GetProfileInt(REG_SECTION, _T("IP_Field3"), 0);

	m_IPCtrl.SetAddress(m_IPAddress.Field0, m_IPAddress.Field1, m_IPAddress.Field2, m_IPAddress.Field3);
	m_httpClient.SetWiimIPaddress(m_IPAddress.Field0, m_IPAddress.Field1, m_IPAddress.Field2, m_IPAddress.Field3); // (192.168.0.228 in my case)

	m_StreamsFilepath = AfxGetApp()->GetProfileString(REG_SECTION, _T("StreamsFilepath"), _T(""));
	GetDlgItem(IDC_EDIT_STREAMSFILE)->SetWindowText(m_StreamsFilepath);

	// stream url list initialisation
	m_ListStream.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	m_ListStream.InsertColumn(0, _T("category"), LVCFMT_LEFT, 100);
	m_ListStream.InsertColumn(1, _T("name"), LVCFMT_LEFT, 120);
	m_ListStream.InsertColumn(2, _T("url"), LVCFMT_LEFT, 1000);
	LoadStreamUrlList();

	// equaliser presets list initialisation
	m_ListEQ.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	m_ListEQ.InsertColumn(0, _T("equaliser presets"), LVCFMT_LEFT, 190);

	// create bold font once
	CFont* pDlgFont = GetFont();
	if (pDlgFont) {
		LOGFONT lf;
		pDlgFont->GetLogFont(&lf);
		lf.lfWeight = FW_BOLD;
	//	lf.lfHeight -= 2;
		m_HeaderFont.DeleteObject();
		m_HeaderFont.CreateFontIndirect(&lf);
	}
	// apply to both list headers (if present)
	if(CHeaderCtrl* ph1 = m_ListStream.GetHeaderCtrl())
		ph1->SetFont(&m_HeaderFont);
	if(CHeaderCtrl* ph2 = m_ListEQ.GetHeaderCtrl())
		ph2->SetFont(&m_HeaderFont);

//	GetDlgItem(IDC_EDIT_STREAMSFILE)->SetFont(&m_HeaderFont);

	if(GetInfoFromDeviceAndPopulateUI()){
		SelectListItems();
		m_Initialised = 1; // this must after the list control SetItemState/EnsureVisible calls because they will trigger a OnLvnItemchangedListStreamurl event.
	}
	UpdateStatusEditBox();

	return TRUE;  // return TRUE unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below to draw the icon. For MFC 
// applications using the document/view model, this is automatically done for you by the framework.
void CWiiMControllerDlg::OnPaint()
{
	if (IsIconic()){
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
	else{
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags the minimized window.
HCURSOR CWiiMControllerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

int CWiiMControllerDlg::GetInfoFromDeviceAndPopulateUI()
{
	// if this first http request fails, then either the IP address is wrong or the WiiM is offline, in which case the other calls below will fail 
	// as well, so dont attempt calling them, (and as this isnt yet multi-threaded calling them will make the window unresponsive for longer).
	if(m_httpClient.GetPlayerStatus()){
		m_httpClient.GetMetaInfo();
		LoadEqualiserPresetsList(m_httpClient.GetEQList());

		int EqEnabled = -1;
		m_httpClient.GetEqStatus(EqEnabled);
		if(EqEnabled==1 || EqEnabled==0)
			GetDlgItem(IDC_BTN_TOGGLE_EQ)->SetWindowText(EqEnabled ? EQ_ON_TEXT : EQ_OFF_TEXT);
		else
			GetDlgItem(IDC_BTN_TOGGLE_EQ)->SetWindowText(_T("equaliser is ???"));

		return 1;
	}
	return 0;
}

void CWiiMControllerDlg::SelectListItems()
{
	// Hightlight/select the currently playing stream in the list control, by comparing the url's in our list with the url retrieved from the device.
	// This is a bit of a (harmless) hack at moment for 2 reasons - 
	// 1) I cannot find a command in the Wiim http API to retrieve the currently playing stream url, but the 'title' field in the getPlayerStatus response usually contains the url of the currently playing stream.
	// 2) the url in the title field is slightly converted/adjusted from the original url sent to the device, the "%3d" part of a string is converted to =. Below I convert these characters so the comparison works correctly.

	for(int i=0; i<m_StreamURLs.size(); i++){
		CString list_url = m_StreamURLs[i].url;

	//	if(list_url.GetLength() - Utf8(m_httpClient.m_Wiim.Title).length() == 2){
		if(list_url.GetLength() - m_httpClient.m_Wiim.Title.length() == 2){
			int Start = list_url.Find(_T("%3d"));
			if(Start > 0){
				// remove %3d from the string and replace with =
				list_url = list_url.Left(Start) + _T("=") + list_url.Mid(Start + 3);
			}
		}
		if(list_url.Compare(Utf8(m_httpClient.m_Wiim.Title)) == 0){
			m_ListStream.SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
			m_ListStream.EnsureVisible(i, FALSE);
			break;
		}
	}

	// Highlight the currently selected equaliser preset.

	CString CurrentEqName = Utf8(m_httpClient.m_Eq.Name);
	for(int i=0; i<m_EqPresetNames.size(); i++){
		if(m_EqPresetNames[i].Compare(CurrentEqName) == 0){
			m_ListEQ.SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
			m_ListEQ.EnsureVisible(i, FALSE);
			break;
		}
	}
}

void CWiiMControllerDlg::OnBnClickedBtnRefreshStats()
{
	if(m_Initialised){
		m_httpClient.GetPlayerStatus();
		m_httpClient.GetMetaInfo();
		UpdateStatusEditBox();
	}
}

#define MAX_EQ_PRESET_NAME_LEN 256
void CWiiMControllerDlg::LoadEqualiserPresetsList(char *str)
{
	// this function will only be called once, when the window is initialized.
	// example equaliser list (char) string from the device :
	//  "[ \"Flat\", \"Acoustic\", \"Bass Booster\", \"Bass Reducer\", \"Classical\", \"Dance\", \"Deep\", \"Electronic\", \"Game\", \"Hip-Hop\", \"Jazz\", \"Latin\", \"Loudness\", \"Lounge\", \"Movie\", \"Piano\", \"Pop\", \"R&B\", \"Rock\", \"Small Speakers\", \"Spoken Word\", \"Treble Booster\", \"Treble Reducer\", \"Vocal Booster\", \"bass up treble down\", \"low frequencys up\", \"middle down\", \"middle down 2\" ]";
	char TempBuf[MAX_EQ_PRESET_NAME_LEN];
	CString csTemp;

	m_ListEQ.DeleteAllItems();
	m_EqPresetNames.clear();
	m_EqPresetNames.reserve(100); // reserve some space to avoid multiple reallocations, (100 presets is unlikely).
	
	while(str){
		char *start = strchr(str, '\"');
		if(!start)
			break;
		char *end = strchr(start + 1, '\"');
		if(!end)
			break;
		size_t len = (end - start) - 1;
		if(len+1 < sizeof(TempBuf)){
			if(strncpy_s(TempBuf, sizeof(TempBuf), start+1, len) == 0){
				TempBuf[len] = '\0';	// null-terminate the string
				csTemp = TempBuf;
				m_ListEQ.InsertItem((int)m_EqPresetNames.size(), csTemp);
				m_EqPresetNames.push_back(csTemp);
			}
		}
		else{
			// this will not happen. (to do - display preset name in msg box).
			AfxMessageBox(_T("\r\n Error reading equaliser presets list. Preset name too long. Preset name skipped"));
		}
		str = end + 1;
	}
}

int CWiiMControllerDlg::LoadStreamUrlsFromFile(const CString& filePath)
{
	m_UrlsErrorLog.Empty();

	CStdioFile file;
	if(!file.Open(filePath, CFile::modeRead | CFile::typeText)){
		m_UrlsErrorLog += _T("\r\n Error opening custom stream urls file : ") + filePath;
		return 0;
	}

	m_ListStream.DeleteAllItems(); // Clear existing items before loading new ones
	m_StreamURLs.clear();
	m_StreamURLs.reserve(256); // reserve some space to avoid multiple reallocations.
	int Ok = 1;
	CString line, last_category = _T("");

	while(file.ReadString(line) && Ok){
		line.Trim();			// Remove (leading/trailing) whitespace
		if(!line.IsEmpty()){	// skip empty lines
			if(line[0] == '#'){
				// store the category name without the '#' character
				last_category = line.Mid(1).Trim();
			}
			else{
				// name and url
				CString name = line;
				// read URL line
				while(file.ReadString(line) && Ok){
					line.Trim();			// Remove (leading/trailing) whitespace
					if(!line.IsEmpty()){
						if(line.Find(_T("http")) != 0){
							m_UrlsErrorLog += _T("\r\nFormatting error in custom stream urls .txt file. \r\nURL expected on the line after stream name <") + name + _T(">. \r\nList incomplete - stopping loading further entries.");
							Ok = 0;
						}
						else{
							// success - got the url line
							break;
						}
					}
				}
				if(Ok)
					m_StreamURLs.push_back({last_category, name, line});
			}
		}
	}
	file.Close();
	return 1;	// gonna return 1 even if there were some errors reading the file, therefore any entries that were successfully read will be displayed.
}

void CWiiMControllerDlg::OnLvnItemchangedListStreamurl(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: Add your control notification handler code here
	*pResult = 0;
	if(m_Initialised){
		m_ListStream_SelectedIndex = pNMLV->iItem;
		TRACE("\n\nOnLvnItemchangedListStreamurl %d", m_ListStream_SelectedIndex); // index of the changed item
		std::string url = CT2A(m_StreamURLs[m_ListStream_SelectedIndex].url);
		m_httpClient.PlayUrl(url);
		UpdateStatusEditBox();
	}
}

void CWiiMControllerDlg::OnLvnItemchangedListEq(NMHDR *pNMHDR,LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: Add your control notification handler code here
	*pResult = 0;
	if(m_Initialised){
		char Name[MAX_EQ_PRESET_NAME_LEN] = {0};
		if(strcpy_s(Name, sizeof(Name)-1, CT2A(m_EqPresetNames[pNMLV->iItem])) == 0){
			// replace ' ' with '+' in the equaliser profile name, as this is what the device expects for some reason, even though the presets list sent by the device 
			// have spaces in their names, when sending a command to change to one of those presets it expects the spaces to be replaced with + symbols.
			for(int i=0; Name[i]; i++)
				if(Name[i] == ' ')
					Name[i] = '+';
			if(m_httpClient.EQLoad(Name))
				GetDlgItem(IDC_BTN_TOGGLE_EQ)->SetWindowText(EQ_ON_TEXT);
		}
	}
}

void CWiiMControllerDlg::OnBnClickedBtnToggleEq()
{
	if(m_Initialised){
		int EqEnabled = m_httpClient.ToggleEqualiserOnOff();
		GetDlgItem(IDC_BTN_TOGGLE_EQ)->SetWindowText(EqEnabled ? EQ_ON_TEXT : EQ_OFF_TEXT);
	}
}

void CWiiMControllerDlg::LoadStreamUrlList()
{
	if(LoadStreamUrlsFromFile(m_StreamsFilepath)){
		for(int i=0; i<m_StreamURLs.size(); i++){
			int item = m_ListStream.InsertItem(i, m_StreamURLs[i].category);
			m_ListStream.SetItemText(item, 1, m_StreamURLs[i].name);
			m_ListStream.SetItemText(item, 2, m_StreamURLs[i].url);
		}
	}
}

#define UNSET_URL_TEXT	_T("Set the IP address of your WiiM device in the control above (restart app to apply new IP).")
#define UNSET_PATH_TEXT	_T("Use the 'browse' button at the bottom to select the .txt file containing the list of custom stream urls.\r\n   (See the example internet_radio.txt provided on GitHub)")
void CWiiMControllerDlg::UpdateStatusEditBox()
{
	// Error messages to be displayed in the UI. (This has become a bit complicated).
	CString error;

	if(m_StreamsFilepath.IsEmpty() && !m_httpClient.m_IPset)
		error.Format(_T("\r\nOn first use of this app you must:\r\n\r\n1) %s\r\n2) %s\r\n"), UNSET_URL_TEXT, UNSET_PATH_TEXT);
	else if(!m_httpClient.m_IPset)
		error.Format(_T("\r\nDevice IP not set: \r\n%s\r\n"), UNSET_URL_TEXT);
	else if(m_StreamsFilepath.IsEmpty())
		error.Format(_T("\r\nFile not selected: \r\n%s\r\n"), UNSET_PATH_TEXT);

	if(!error.IsEmpty()){
		// first usage of app - error messages and instructions.
		GetDlgItem(IDC_EDIT_STATUS)->SetWindowText(error);
		m_UrlsErrorLog.Empty(); // any errors in here arent needed/wanted now.
	}
	else{
		if(m_httpClient.GetNewError(error) || !m_UrlsErrorLog.IsEmpty()){
			if(!m_UrlsErrorLog.IsEmpty())
				error += m_UrlsErrorLog;
			GetDlgItem(IDC_EDIT_STATUS)->SetWindowText(error);
		}
		else{
			UpdatePlayerStatusString();
			GetDlgItem(IDC_EDIT_STATUS)->SetWindowText(m_LastStatus);
		}
	}
}

void CWiiMControllerDlg::UpdatePlayerStatusString()
{
	m_LastStatus.Empty();

	if(m_httpClient.m_Wiim.status!="play" && m_httpClient.m_Wiim.status!="pause" && m_httpClient.m_Wiim.status!="stop")
		m_LastStatus = _T("\r\n Ready, select a stream to play from the list below...");
	else{

		if(m_httpClient.m_Wiim.vendor == "CustomPushUrl"){
			// raw stream url. When getMetaData request is used url is put in title, but it also alters some characters, so use the original url from the list.
			CString Title = Utf8(m_httpClient.m_Wiim.Title);
			if(m_ListStream_SelectedIndex != -1)
				Title = m_StreamURLs[m_ListStream_SelectedIndex].url;

			m_LastStatus.Format(_T("Vendor: %s\r\nStream URL: %s\r\nPosition: %s\r\nStatus: %s\r\nVolume: %d, Mute: %d"),
				Utf8(m_httpClient.m_Wiim.vendor),
				Title,
				Utf8(m_httpClient.m_Wiim.curpos_fmt),
				Utf8(m_httpClient.m_Wiim.status),
				m_httpClient.m_Wiim.vol, m_httpClient.m_Wiim.mute
			);
		}
		else{
			m_LastStatus.Format(_T("Vendor: %s\r\nTitle: %s\r\nArtist: %s\r\nPosition: %s / %s\r\nStatus: %s, Playlist: %d / %d\r\nVolume: %d, Mute: %d"),
				Utf8(m_httpClient.m_Wiim.vendor),
				Utf8(m_httpClient.m_Wiim.Title),
				Utf8(m_httpClient.m_Wiim.Artist),
				Utf8(m_httpClient.m_Wiim.curpos_fmt),
				Utf8(m_httpClient.m_Wiim.totlen_fmt),
				Utf8(m_httpClient.m_Wiim.status),
				m_httpClient.m_Wiim.plicurr, m_httpClient.m_Wiim.plicount,
				m_httpClient.m_Wiim.vol, m_httpClient.m_Wiim.mute
			);
		}

		if(m_httpClient.m_Wiim.sampleRate!="" && m_httpClient.m_Wiim.bitDepth!="" && m_httpClient.m_Wiim.bitRate!=""){
			CString MetaInfoStr;
			MetaInfoStr.Format(_T("\r\nSample Rate: %s Hz, Bit Depth: %s bit, Bit Rate: %s kb/s"), Utf8(m_httpClient.m_Wiim.sampleRate), Utf8(m_httpClient.m_Wiim.bitDepth), Utf8(m_httpClient.m_Wiim.bitRate));
			m_LastStatus += MetaInfoStr;
		}
		else
			m_LastStatus += _T("\r\n Press 'refresh info' button for [sampleRate, bitDepth, bitRate]");
	}
}

void CWiiMControllerDlg::OnBnClickedBtnLoadFile()
{
	LoadStreamUrlList();
	UpdateStatusEditBox();
}

void CWiiMControllerDlg::OnBnClickedBtnBrowse()
{
	CFileDialog FileDlg(TRUE, _T("txt"), nullptr, OFN_FILEMUSTEXIST, _T("Text Files (*.txt)|*.txt|All Files (*.*)|*.*||"));
	if(FileDlg.DoModal() == IDOK){
		m_StreamsFilepath = FileDlg.GetPathName();
		GetDlgItem(IDC_EDIT_STREAMSFILE)->SetWindowText(m_StreamsFilepath);
		AfxGetApp()->WriteProfileString(REG_SECTION, _T("StreamsFilepath"), m_StreamsFilepath);
		OnBnClickedBtnLoadFile();
	}
}

void CWiiMControllerDlg::OnBnClickedBtnVolUp()
{
	if(m_Initialised){
		m_httpClient.VolumeStep(1);
		UpdateStatusEditBox();
	}
}

void CWiiMControllerDlg::OnBnClickedBtnVolDown()
{
	if(m_Initialised){
		m_httpClient.VolumeStep(-1);
		UpdateStatusEditBox();
	}
}

void CWiiMControllerDlg::OnBnClickedBtnMute()
{
	if(m_Initialised){
		m_httpClient.ToggleMute();
		UpdateStatusEditBox();
	}
}

void CWiiMControllerDlg::OnBnClickedBtnTogglePlay()
{
	if(m_Initialised){
		m_httpClient.TogglePlay();
		UpdateStatusEditBox();
	}
}

void CWiiMControllerDlg::OnIpnFieldchangedIpaddressWiim(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMIPADDRESS pIPAddr = reinterpret_cast<LPNMIPADDRESS>(pNMHDR);
	// TODO: Add your control notification handler code here
	*pResult = 0;
	// not using this... instead I will retrieve the IP address in DestroyWindow and save it to the registry there.
}

BOOL CWiiMControllerDlg::DestroyWindow()
{
	IPADDRESS Temp;
	m_IPCtrl.GetAddress(Temp.Field0, Temp.Field1, Temp.Field2, Temp.Field3);
	if(Temp.Field0 != m_IPAddress.Field0)
		AfxGetApp()->WriteProfileInt(REG_SECTION, _T("IP_Field0"), Temp.Field0);
	if(Temp.Field1 != m_IPAddress.Field1)
		AfxGetApp()->WriteProfileInt(REG_SECTION, _T("IP_Field1"), Temp.Field1);
	if(Temp.Field2 != m_IPAddress.Field2)
		AfxGetApp()->WriteProfileInt(REG_SECTION, _T("IP_Field2"), Temp.Field2);
	if(Temp.Field3 != m_IPAddress.Field3)
		AfxGetApp()->WriteProfileInt(REG_SECTION, _T("IP_Field3"), Temp.Field3);
	return CDialog::DestroyWindow();
}

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
