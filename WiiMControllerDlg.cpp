// WiiMControllerDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "WiiMController.h"
#include "WiiMControllerDlg.h"
//#include "afxdialogex.h"
#include "tools.h"
#include <fstream>

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
	m_ListEQ_SelectedIndex = -1;
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
	ON_WM_DESTROY()

	ON_NOTIFY(LVN_ITEMCHANGED,IDC_LIST_STREAMURL,&CWiiMControllerDlg::OnLvnItemchangedListStreamurl)
	ON_NOTIFY(LVN_ITEMCHANGED,IDC_LIST_EQ,&CWiiMControllerDlg::OnLvnItemchangedListEq)
	ON_NOTIFY(IPN_FIELDCHANGED,IDC_IPADDRESS_WIIM,&CWiiMControllerDlg::OnIpnFieldchangedIpaddressWiim)

	ON_BN_CLICKED(IDC_BTN_LOAD_FILE,&CWiiMControllerDlg::OnBnClickedBtnLoadFile)
	ON_BN_CLICKED(IDC_BTN_OPEN,&CWiiMControllerDlg::OnBnClickedBtnOpen)
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

	// stream url list initialisation
	m_ListStream.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	m_ListStream.InsertColumn(0, _T("category"), LVCFMT_LEFT, 100);
	m_ListStream.InsertColumn(1, _T("name"), LVCFMT_LEFT, 120);
	m_ListStream.InsertColumn(2, _T("stream url"), LVCFMT_LEFT, 1000);

	m_StreamsFilepath = AfxGetApp()->GetProfileString(REG_SECTION, _T("StreamsFilepath"), _T(""));
	GetDlgItem(IDC_EDIT_STREAMSFILE)->SetWindowText(m_StreamsFilepath);
	LoadStreamUrlList();

	// equaliser presets list initialisation
	m_ListEQ.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	m_ListEQ.InsertColumn(0, _T("equaliser presets"), LVCFMT_LEFT, 190);

	if(GetInfoFromDeviceAndPopulateUI()){
		SelectListItems();
		m_Initialised = 1; // this must after the list control SetItemState/EnsureVisible calls because they will trigger a OnLvnItemchangedListStreamurl event.
	}
	
	ApplyCustomFont();
	RestoreWindowPos();
	UpdateStatusEditBox();

	return TRUE;  // return TRUE unless you set the focus to a control
}

void CWiiMControllerDlg::ApplyCustomFont()
{
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

	GetDlgItem(IDC_EDIT_STREAMSFILE)->SetFont(&m_HeaderFont);
}

void CWiiMControllerDlg::RestoreWindowPos()
{
	// restore window pos/size if saved
	const int UNSET = -99999;
	int left = AfxGetApp()->GetProfileInt(REG_SECTION, _T("Left"), UNSET);
	if(left != UNSET){
		int top    = AfxGetApp()->GetProfileInt(REG_SECTION, _T("Top"), 0);
		int width  = AfxGetApp()->GetProfileInt(REG_SECTION, _T("Width"), 800);
		int height = AfxGetApp()->GetProfileInt(REG_SECTION, _T("Height"), 600);
		CRect rc(left, top, left + width, top + height);
		// Ensure rect maps to a monitor (avoid restoring off-screen)
		HMONITOR hm = MonitorFromRect(rc, MONITOR_DEFAULTTONULL);
		if(hm){
			MoveWindow(&rc);
			int show = AfxGetApp()->GetProfileInt(REG_SECTION, _T("ShowCmd"), SW_SHOWNORMAL);
			if(show == SW_SHOWMAXIMIZED || show == SW_MAXIMIZE)
				ShowWindow(SW_MAXIMIZE);
		}
	}
}

void CWiiMControllerDlg::OnDestroy()
{
	// capture IP before calling base
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

	// capture placement before calling base
	WINDOWPLACEMENT wp = {};
	wp.length = sizeof(wp);
	if (GetWindowPlacement(&wp)) {
		// wp.rcNormalPosition is the normal (restored) rectangle
		CRect r = wp.rcNormalPosition;
		AfxGetApp()->WriteProfileInt(REG_SECTION, _T("Left"), r.left);
		AfxGetApp()->WriteProfileInt(REG_SECTION, _T("Top"), r.top);
		AfxGetApp()->WriteProfileInt(REG_SECTION, _T("Width"), r.Width());
		AfxGetApp()->WriteProfileInt(REG_SECTION, _T("Height"), r.Height());
		AfxGetApp()->WriteProfileInt(REG_SECTION, _T("ShowCmd"), wp.showCmd);
	}

	CDialog::OnDestroy();
	// TODO: Add your message handler code here
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
	// if this first http request fails, then either the IP address is wrong or the WiiM is offline.
	if(m_httpClient.GetPlayerStatus()){
		
	//	m_httpClient.GetMetaInfo();  not essential when app is opened, and it adds a bit of lag, so don't call it here.
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
	// Hightlight/select the currently playing stream in the list control, by comparing the url's in our list with the url retrieved from the device. I cannot find a command in 
	// the Wiim http API to retrieve the currently playing stream url, but the 'title' field in the getPlayerStatus response usually contains the url of the currently playing stream.
	for(int i=0; i<m_StreamURLs.size(); i++){
		if(m_StreamURLs[i].url == m_httpClient.m_Wiim.Title){
			m_ListStream.SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
			m_ListStream.EnsureVisible(i, FALSE);
			m_ListStream_SelectedIndex = i;
			break;
		}
	}

	// Highlight/select the currently selected equaliser preset.
	for(int i=0; i<m_EqPresetNames.size(); i++){
		if(m_EqPresetNames[i] == m_httpClient.m_Eq.Name){
	//	if(m_EqPresetNames[i].compare(m_httpClient.m_Eq.Name) == 0){
			m_ListEQ.SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
			m_ListEQ.EnsureVisible(i, FALSE);
			m_ListEQ_SelectedIndex = i;
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

void CWiiMControllerDlg::LoadEqualiserPresetsList(char *str)
{
	// this function will only be called once, when the window is initialized.
	// example equaliser list (char) string from the device :
	//  "[ \"Flat\", \"Acoustic\", \"Bass Booster\", \"Bass Reducer\", \"Classical\", \"Dance\", \"Deep\", \"Electronic\", \"Game\", \"Hip-Hop\", \"Jazz\", \"Latin\", \"Loudness\", \"Lounge\", \"Movie\", \"Piano\", \"Pop\", \"R&B\", \"Rock\", \"Small Speakers\", \"Spoken Word\", \"Treble Booster\", \"Treble Reducer\", \"Vocal Booster\", \"bass up treble down\", \"low frequencys up\", \"middle down\", \"middle down 2\" ]";
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

		if(len > 0){
			// use std::string to copy exactly 'len' chars
			std::string preset(start + 1, len);
			m_ListEQ.InsertItem((int)m_EqPresetNames.size(), Utf8(preset));
			m_EqPresetNames.push_back(preset);
		}
		str = end + 1;
	}
}

void CWiiMControllerDlg::TrimString(std::string &s)
{
	// Remove (leading/trailing) whitespace, and other non-printable characters (eg, control characters like '\r'), from the start and end of the string.
	while(!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) s.erase(s.begin());
	while(!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) s.pop_back();
}

int CWiiMControllerDlg::HasBOM(std::string &line)
{
	// Check for UTF-8 BOM on the first line of the file, and if found, remove it. 
	// The UTF-8 BOM consists of the byte sequence 0xEF, 0xBB, 0xBF at the start of the file. 
	if(!line.empty() && line.size()>=3 &&
		static_cast<unsigned char>(line[0]) == 0xEF && 
		static_cast<unsigned char>(line[1]) == 0xBB &&
		static_cast<unsigned char>(line[2]) == 0xBF)
	{
		line.erase(0, 3);
		return 1; // BOM found
	}
	return 0; // No BOM
}

// Text file must be UTF-8 encoded, and if it has a BOM at the start of the file, it will be skipped. Each stream entry in the file should consist 
// of a name line followed by a url line, with optional category lines starting with '#' to group streams. Empty lines are ignored.
bool CWiiMControllerDlg::LoadStreamUrlsFromUtf8File()
{
	m_UrlsErrorLog.Empty();

	std::wstring wpath = static_cast<LPCTSTR>(m_StreamsFilepath);
	std::ifstream infile(wpath, std::ios::binary);
	// open file in binary mode to ensure we read the raw bytes which is important for correctly handling UTF-8 encoded text files. 
	// std::ifstream can handle wide-character paths on Windows, so we can pass the wide string directly without converting it to UTF-8.
	if(!infile.is_open()){
		m_UrlsErrorLog += _T("\r\nError opening direct stream urls file : \r\n     ") + m_StreamsFilepath;
		return false;
	}

	m_ListStream.DeleteAllItems(); // Clear existing items before loading new ones
	m_StreamURLs.clear();
	m_StreamURLs.reserve(256); // reserve some space to avoid multiple reallocations.
	bool Ok = true, FirstLine = true;
	std::string line, last_category = "";

	// UTF-8 itself does not break std::getline(), because UTF-8 is byte-oriented and newline remains normal ASCII.
	while(std::getline(infile, line) && Ok){
		if(FirstLine){
			FirstLine = false;
			if(HasBOM(line))	// skip UTF-8 BOM if present at the start of the file  (text.starts_with("\xEF\xBB\xBF"))
				continue;
		}
		TrimString(line);
		if(!line.empty()){    // skip empty lines
			if(line[0] == '#'){
				// store the category name without the '#' character
				last_category = line.substr(1);
				TrimString(last_category);
			}
			else{
				// name and url
				std::string url, name = line;
				// read URL line
				while(std::getline(infile, line) && Ok){
					TrimString(line);
					if(!line.empty()){
						if(!line.starts_with("http://") && !line.starts_with("https://")){
							m_UrlsErrorLog += _T("Formatting error in - ") + m_StreamsFilepath + _T(" \r\nThe following name/url combination has an invalid URL:\r\n\r\nname:\t") + 
												Utf8(name) + _T("\r\nurl:\t") + Utf8(line) + _T("\r\n\r\nFIX THE ERRORS then press the 'refresh list' button to reload the list");
							Ok = false;
						}
						else{
							// success - got the url line
							url = line;
							break;
						}
					}
				}
				if(Ok)
					m_StreamURLs.push_back({last_category, name, url});
			}
		}
	}
	infile.close();
	return Ok;
}

void CWiiMControllerDlg::OnLvnItemchangedListStreamurl(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: Add your control notification handler code here
	*pResult = 0;
	if(m_Initialised){
		if(m_ListStream_SelectedIndex != pNMLV->iItem){
			if(pNMLV->iItem>=0 && pNMLV->iItem<(int)(m_StreamURLs.size())){

				m_ListStream_SelectedIndex = pNMLV->iItem;
				TRACE("\n\nItemchanged ListStreamurl %d  ", m_ListStream_SelectedIndex);
				m_httpClient.PlayUrl(m_StreamURLs[m_ListStream_SelectedIndex].url);
				UpdateStatusEditBox();
			}
		}
	}
}

void CWiiMControllerDlg::OnLvnItemchangedListEq(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: Add your control notification handler code here
	*pResult = 0;
	if(m_Initialised){
		if(m_ListEQ_SelectedIndex != pNMLV->iItem){
			if(pNMLV->iItem>=0 && pNMLV->iItem<(int)(m_EqPresetNames.size())){

				m_ListEQ_SelectedIndex = pNMLV->iItem;
				TRACE("\n\nItemchanged ListEq %d  ", m_ListEQ_SelectedIndex);
				std::string name = m_EqPresetNames[pNMLV->iItem];
				// replace ' ' with '+' in the equaliser profile name, as this is what the device expects for some reason, even though the presets list sent by the device 
				// have spaces in their names, when sending a command to change to one of those presets it expects the spaces to be replaced with + symbols.
				for(size_t i=0; i<name.length(); ++i)
					if(name[i] == ' ')
						name[i] = '+';

				if(m_httpClient.EQLoad(name))
					GetDlgItem(IDC_BTN_TOGGLE_EQ)->SetWindowText(EQ_ON_TEXT);
			}
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
	if(LoadStreamUrlsFromUtf8File()){
		for(int i=0; i<m_StreamURLs.size(); i++){
			int item = m_ListStream.InsertItem(i, Utf8(m_StreamURLs[i].category));
			m_ListStream.SetItemText(item, 1, Utf8(m_StreamURLs[i].name));
			m_ListStream.SetItemText(item, 2, Utf8(m_StreamURLs[i].url));
		}
	}
}

#define UNSET_URL_TEXT	_T("Set the IP address of your WiiM device in the control above (restart app to apply new IP).")
#define UNSET_PATH_TEXT	_T("Press 'open' button to open the file containing the list of direct stream urls.\r\n   (See example .txt file <internet_radio.txt> provided on GitHub)")
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
		std::string httpError;
		if(m_httpClient.GetNewError(httpError) || !m_UrlsErrorLog.IsEmpty()){
			error = Utf8(httpError) + m_UrlsErrorLog;
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

	if(m_httpClient.m_Wiim.status == "none"){
		m_LastStatus = _T("\r\n Ready, select a stream to play from the list below...");
	}
	else{

		if(m_httpClient.m_Wiim.vendor == "CustomPushUrl"){
			// raw stream url. When getMetaData request is used url is put in title, but it also alters some characters, so use the original url from the list.
			CString Title = Utf8(m_httpClient.m_Wiim.Title);
			if(m_ListStream_SelectedIndex != -1)
				Title = Utf8(m_StreamURLs[m_ListStream_SelectedIndex].url);

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

void CWiiMControllerDlg::OnBnClickedBtnOpen()
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
	// not using this... instead I will retrieve the IP address in the destroy window handler and save it to the registry there.
}
