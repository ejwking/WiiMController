
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
#include "tools.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define REG_SECTION	_T("WiimCntrlWnd")

CWiiMControllerDlg::CWiiMControllerDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_WIIMCONTROLLER_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_NumStream = 0;
	m_NumEQ = 0;
	m_InitUI = 0;

	m_httpClient.SetBaseUrl("192.168.0.228");
	m_DeviceAvailable = 1;	// temp until discovery is implemented, just assume the device is available for now.
}

void CWiiMControllerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_STREAMURL, m_ListStream);
	DDX_Control(pDX, IDC_LIST_EQ, m_ListEQ);
}

BEGIN_MESSAGE_MAP(CWiiMControllerDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	
	ON_NOTIFY(LVN_ITEMCHANGED,IDC_LIST_STREAMURL,&CWiiMControllerDlg::OnLvnItemchangedListStreamurl)
	ON_NOTIFY(LVN_ITEMCHANGED,IDC_LIST_EQ,&CWiiMControllerDlg::OnLvnItemchangedListEq)

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
//	LoadEditboxFont(GetDlgItem(IDC_EDIT_STATUS));

	m_StreamsFilepath = AfxGetApp()->GetProfileString(REG_SECTION, _T("StreamsFilepath"), _T(""));
	GetDlgItem(IDC_EDIT_STREAMSFILE)->SetWindowText(m_StreamsFilepath);

	// stream url list initialisation
	m_ListStream.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	m_ListStream.InsertColumn(0, _T("category"), LVCFMT_LEFT, 80);
	m_ListStream.InsertColumn(1, _T("name"), LVCFMT_LEFT, 120);
	m_ListStream.InsertColumn(2, _T("url"), LVCFMT_LEFT, 1000);
	LoadStreamUrlList();

	// equaliser presets list initialisation
	m_ListEQ.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	m_ListEQ.InsertColumn(0, _T("equaliser presets"), LVCFMT_LEFT, 200);

	// create bold font once
	CFont* pDlgFont = GetFont();
	if (pDlgFont) {
		LOGFONT lf;
		pDlgFont->GetLogFont(&lf);
		lf.lfWeight = FW_BOLD;
		m_HeaderFont.DeleteObject();
		m_HeaderFont.CreateFontIndirect(&lf);
	}
	// apply to both list headers (if present)
	if(CHeaderCtrl* ph1 = m_ListStream.GetHeaderCtrl())
		ph1->SetFont(&m_HeaderFont);
	if(CHeaderCtrl* ph2 = m_ListEQ.GetHeaderCtrl())
		ph2->SetFont(&m_HeaderFont);

	GetInfoFromDevice();

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below to draw the icon. For MFC 
// applications using the document/view model, this is automatically done for you by the framework.
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

void CWiiMControllerDlg::GetInfoFromDevice()
{
	if(m_DeviceAvailable){
		m_httpClient.GetPlayerStatus();
		m_httpClient.GetMetaInfo(); // this just kicked up curl error when i first opened app, investigate ..

		UpdateStatusEditBox();
		/* the text in m_Wiim.Title is slightly converted/adjusted from the original Title string sent to the device, the % symbol has been converted to =, I am not sure why 
			but it means this comparison doesnt work properly, so I cant highlight the currently playing stream when the app starts up. ToDo investigate and fix.
		for(int i=0; i<m_NumStream; i++){
			if(m_StreamInfo[i].url.Compare(Utf8(m_httpClient.m_Wiim.Title)) == 0){
				m_ListStream.SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
				m_ListStream.EnsureVisible(i, FALSE);
				break;
			}
		} */

		// Get the equaliser presets list, and the current equaliser status, so we can populate the equaliser presets list and highlight the currently selected preset.
		LoadEqualiserPresetsList(m_httpClient.GetEQList());
		CString CurrentEqName;
		int EqEnabled = -1;
		m_httpClient.GetEqStatus(EqEnabled, CurrentEqName);
		if(EqEnabled==1 || EqEnabled==0)
			GetDlgItem(IDC_BTN_TOGGLE_EQ)->SetWindowText(EqEnabled ? _T("equaliser is ON") : _T("equaliser is OFF"));
		else
			GetDlgItem(IDC_BTN_TOGGLE_EQ)->SetWindowText(_T("equaliser is ???"));

		for(int i=0; i<m_NumEQ; i++){
			if(m_EQname[i].Compare(CurrentEqName) == 0){
				m_ListEQ.SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
				m_ListEQ.EnsureVisible(i, FALSE);
				break;
			}
		}
		m_InitUI = 1; // this must come last because m_ListEQ.EnsureVisible above will trigger a OnLvnItemchangedListStreamurl event.
	}
}

void CWiiMControllerDlg::OnBnClickedBtnRefreshStats()
{
	if(m_DeviceAvailable && m_InitUI){
		m_httpClient.GetPlayerStatus();
		m_httpClient.GetMetaInfo();
		UpdateStatusEditBox();
	}
}

void CWiiMControllerDlg::OnBnClickedBtnToggleEq()
{
	if(m_DeviceAvailable && m_InitUI){
		int EqEnabled = m_httpClient.ToggleEqualiserOnOff();
		GetDlgItem(IDC_BTN_TOGGLE_EQ)->SetWindowText(EqEnabled ? _T("equaliser is ON") : _T("equaliser is OFF"));
	}
}

void CWiiMControllerDlg::LoadEqualiserPresetsList(char *str)
{
	// example:  "[ \"Flat\", \"Acoustic\", \"Bass Booster\", \"Bass Reducer\", \"Classical\", \"Dance\", \"Deep\", \"Electronic\", \"Game\", \"Hip-Hop\", \"Jazz\", \"Latin\", \"Loudness\", \"Lounge\", \"Movie\", \"Piano\", \"Pop\", \"R&B\", \"Rock\", \"Small Speakers\", \"Spoken Word\", \"Treble Booster\", \"Treble Reducer\", \"Vocal Booster\", \"bass up treble down\", \"low frequencys up\", \"middle down\", \"middle down 2\" ]";
	char TempBuf[256];
	m_ListEQ.DeleteAllItems();
	m_NumEQ = 0;
	while(str && m_NumEQ<EQ_MAX){
		char *start = strchr(str, '\"');
		if(!start)
			break;
		char *end = strchr(start + 1, '\"');
		if(!end)
			break;
		size_t len = (end - start) - 1;
		if(len+1 < sizeof(TempBuf)){
			strncpy_s(TempBuf, sizeof(TempBuf), start+1, len);
			TempBuf[len] = '\0'; // null-terminate the string
			m_EQname[m_NumEQ] = TempBuf; // assign to CString, which will handle memory management
			m_ListEQ.InsertItem(m_NumEQ, m_EQname[m_NumEQ]);
			m_NumEQ++;
		}
		else{
			AfxMessageBox(_T("error - Equalise Preset name too long"));
			break;
		}
		str = end + 1;
	}
}

int CWiiMControllerDlg::LoadStreamUrlsFromFile(const CString& filePath)
{
	CStdioFile file;
	if(!file.Open(filePath, CFile::modeRead | CFile::typeText))
		return 0;

	m_ListStream.DeleteAllItems(); // Clear existing items before loading new ones
	int Ok = 1;
	CString line, last_category = _T("");
	m_NumStream = 0;
	while(file.ReadString(line) && Ok){
		line.Trim();			// Remove (leading/trailing) whitespace
		if(!line.IsEmpty()){	// skip empty lines
			if(line[0] == '#'){
				// store the category name without the '#' character
				last_category = line.Mid(1).Trim();
			}
			else{
				// name and url
				if(m_NumStream < LIST_MAX){
					CString name = line;
					// Read URL line
					while(file.ReadString(line) && Ok){
						line.Trim();			// Remove (leading/trailing) whitespace
						if(!line.IsEmpty()){
							if(line.Find(_T("http")) != 0){
								AfxMessageBox(_T("Error reading streams .txt file.\r\n Malformed file - expected URL after name. Stopping loading further entries."));
								Ok = 0;
							}
							else
								break; // got the url line
						}
					}
					m_StreamInfo[m_NumStream].category = last_category;
					m_StreamInfo[m_NumStream].name = name;
					m_StreamInfo[m_NumStream].url  = line;
					m_NumStream++;
				}
				else{
					AfxMessageBox(_T("Error - Reached maximum list capacity. TO DO - MAKE THIS DYNAMIC"));
					Ok = 0;
				}
			}
		}
	}
	file.Close();
	return Ok;
}

void CWiiMControllerDlg::OnLvnItemchangedListStreamurl(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: Add your control notification handler code here
	*pResult = 0;
	if(m_DeviceAvailable && m_InitUI){
		TRACE("\n\nOnLvnItemchangedListStreamurl %d", pNMLV->iItem); // index of the changed item
		std::string url = CT2A(m_StreamInfo[pNMLV->iItem].url);
		m_httpClient.PlayUrl(url);
		m_httpClient.ResetMetaInfo(); // reset meta info so that old info from previous stream doesnt briefly show until the new info is retrieved from the device.
		UpdateStatusEditBox();
	}
}

void CWiiMControllerDlg::OnLvnItemchangedListEq(NMHDR *pNMHDR,LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: Add your control notification handler code here
	*pResult = 0;
	if(m_DeviceAvailable && m_InitUI){
		char Name[256] = {0};
		strcpy_s(Name, sizeof(Name)-1, CT2A(m_EQname[pNMLV->iItem]));
		// replace ' ' with '+' in the equaliser profile name, as this is what the device expects for some reason, even though the presets list sent by the device 
		// have spaces in their names, when sending a command to change to one of those presets it expects the spaces to be replaced with + symbols.
		for(int i=0; Name[i]; i++)
			if(Name[i] == ' ')
				Name[i] = '+';
		m_httpClient.EQLoad(Name);
	}
}

void CWiiMControllerDlg::LoadStreamUrlList()
{
	if(PathFileExists(m_StreamsFilepath))
		if(LoadStreamUrlsFromFile(m_StreamsFilepath))
			for(int i=0; i<m_NumStream; i++){
				int item = m_ListStream.InsertItem(i, m_StreamInfo[i].category);
				m_ListStream.SetItemText(item, 1, m_StreamInfo[i].name);
				m_ListStream.SetItemText(item, 2, m_StreamInfo[i].url);
			}
}

void CWiiMControllerDlg::UpdateStatusEditBox()
{
	UpdatePlayerStatusString();
	GetDlgItem(IDC_EDIT_STATUS)->SetWindowText(m_LastStatus);
}

void CWiiMControllerDlg::UpdatePlayerStatusString()
{
	m_LastStatus.Empty();
//	std::string title = (m_httpClient.m_Wiim.vendor == "CustomPushUrl") ? "Custom URL" : m_httpClient.m_Wiim.Title;
	std::string title = m_httpClient.m_Wiim.Title;

	m_LastStatus.Format(_T("Vendor: %s\r\nTitle: %s\r\nArtist: %s\r\nAlbum: %s\r\nPosition: %s / %s\r\nStatus: %s\r\nPlaylist: %d / %d\r\nVolume: %d\r\nMute: %d"),
		Utf8(m_httpClient.m_Wiim.vendor),
		Utf8(title),
		Utf8(m_httpClient.m_Wiim.Artist),
		Utf8(m_httpClient.m_Wiim.Album),
		Utf8(m_httpClient.m_Wiim.curpos_fmt),
		Utf8(m_httpClient.m_Wiim.totlen_fmt),
		Utf8(m_httpClient.m_Wiim.status),
		m_httpClient.m_Wiim.plicurr, m_httpClient.m_Wiim.plicount,
		m_httpClient.m_Wiim.vol, m_httpClient.m_Wiim.mute
	);

	if(m_httpClient.m_Wiim.sampleRate!="" && m_httpClient.m_Wiim.bitDepth!="" && m_httpClient.m_Wiim.bitRate!=""){
		CString MetaInfoStr;
		MetaInfoStr.Format(_T("\r\nSample Rate: %sHz, Bit Depth: %sbit, Bit Rate: %skb/s"), Utf8(m_httpClient.m_Wiim.sampleRate), Utf8(m_httpClient.m_Wiim.bitDepth), Utf8(m_httpClient.m_Wiim.bitRate));
		m_LastStatus += MetaInfoStr;
	}
}

void CWiiMControllerDlg::OnBnClickedBtnLoadFile()
{
	LoadStreamUrlList();
}

void CWiiMControllerDlg::OnBnClickedBtnBrowse()
{
	CFileDialog FileDlg(TRUE, _T("txt"), nullptr, OFN_FILEMUSTEXIST, _T("Text Files (*.txt)|*.txt|All Files (*.*)|*.*||"));
	if(FileDlg.DoModal() == IDOK){
		m_StreamsFilepath = FileDlg.GetPathName();
		GetDlgItem(IDC_EDIT_STREAMSFILE)->SetWindowText(m_StreamsFilepath);
		AfxGetApp()->WriteProfileString(REG_SECTION, _T("StreamsFilepath"), m_StreamsFilepath);
	}
}

void CWiiMControllerDlg::OnBnClickedBtnVolUp()
{
	if(m_DeviceAvailable && m_InitUI){
		m_httpClient.VolumeStep(1);
		UpdateStatusEditBox();
	}
}

void CWiiMControllerDlg::OnBnClickedBtnVolDown()
{
	if(m_DeviceAvailable && m_InitUI){
		m_httpClient.VolumeStep(-1);
		UpdateStatusEditBox();
	}
}

void CWiiMControllerDlg::OnBnClickedBtnMute()
{
	if(m_DeviceAvailable && m_InitUI){
		m_httpClient.ToggleMute();
		UpdateStatusEditBox();
	}
}

void CWiiMControllerDlg::OnBnClickedBtnTogglePlay()
{
	if(m_DeviceAvailable && m_InitUI){
		m_httpClient.TogglePlay();
		UpdateStatusEditBox();
	}
}
