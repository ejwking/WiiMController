

#pragma once

#include <string>
#include <curl/curl.h>


// How to install curl and libcurl using vcpkg
// https://curl.se/docs/install.html


/* FIX THIS
warning: The vcpkg C:\dev\vcpkg\vcpkg.exe is using detected vcpkg root C:\dev\vcpkg and ignoring mismatched VCPKG_ROOT 
environment value C:\Program Files\Microsoft Visual Studio\18\Community\VC\vcpkg. To suppress this message, unset the 
environment variable or use the --vcpkg-root command line switch.

ALSO SEE 
https://learn.microsoft.com/en-gb/vcpkg/get_started/get-started-vs?pivots=shell-cmd
which gives CLI command to set the environment variable permanently in the user environment.
*/


/*
Device discovery
WiiM uses SSDP (UPnP discovery).
Good options:
miniupnpc → simple, widely used
Write your own SSDP client (UDP multicast to 239.255.255.250:1900)
*/

struct WIIM_STATUS
{
	int Initialised = 0;

	int type = 0;
	int ch = 0;
	int mode = 0;
	int loop = 0;
	int eq = 0;

	std::string Name, IP, Model, SerialNumber;
	std::string vendor, status, offset_pts;
	std::string Title, Artist, Album, ArtUrl;
	std::string curpos, curpos_fmt;
	std::string totlen, totlen_fmt;

	int alarmflag = 0;
	int plicount = 0;
	int plicurr = 0;
	int vol = 0;
	int mute = 0;
};

struct WIIM_EQUALISER
{
	std::string status, source_name, EQStat, Name, pluginURI, channelMode;
	int EQLevel;
};

struct MEMORYSTRUCT
{
	char *memory;
	size_t mem_size;
	size_t response_size;
};

/*
You can send 'HTTPs Get' request to the device, most of the response is in the JSON format.
Request format is https://x.x.x.x/httpapi.asp?command=********
x.x.x.x is the IP address of the device, ******* is the actual command.

	Play,
	Pause,
	Stop,
	Next,
	Previous,
	Mute,
	VolumeUp,
	VolumeDown
*/


class CWiimHttpClient
{
public:
	CWiimHttpClient();
	~CWiimHttpClient();

	WIIM_STATUS m_Wiim;

	int GetStatusEx();
	int GetPlayerStatus();
	int SetBaseUrl(char *pUrl);
	char *GetEQList();
	int TogglePlay();
	int PlayUrl(const std::string& url);
	int GetEqStatus(int &EqualiserOn, CString &CurrentEqName);
	int EQLoad(char *pName);
	int ToggleMute();
	int VolumeStep(int step); // positive or negative
	int ToggleEqualiserOnOff();

private:
	MEMORYSTRUCT m_data;
	WIIM_EQUALISER m_Eq;

	int m_CurlGlobalInit;
	int m_EqualiserOn = 0;

	int ParseJsonString_PlayerStatus();
	int ParseJsonString_EqBand();
	int HttpRequest(std::string &url);
	void CurlWiimConfig(CURL *curl_handle);
	int CurlGlobalInit_Ok();
	int SendCommand(const std::string& command, const std::string& arg_1="", const std::string& arg_2="", const std::string& arg_3="");

	std::string ExtractArtworkUrl(const std::string& vendor);
	std::string FormatTimeMs(const std::string& msStr);
	std::string HexToUtf8(const std::string& hex);
	std::string UrlDecode(const std::string& src);
};
