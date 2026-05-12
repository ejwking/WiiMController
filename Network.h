

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


struct WIIM_INFO
{
	int Initialised = 0;

	std::string Name, IP, Model, SerialNumber;

	int type = 0;
	int ch = 0;
	int mode = 0;
	int loop = 0;
	int eq = 0;
	int EqualiserOn = 0;

	std::string vendor;
	std::string status;
	std::string curpos;
	std::string offset_pts;
	std::string totlen;

	std::string Title;
	std::string Artist;
	std::string Album;

	std::string curpos_fmt;
	std::string totlen_fmt;
	std::string ArtUrl;

	int alarmflag = 0;
	int plicount = 0;
	int plicurr = 0;
	int vol = 0;
	int mute = 0;
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

https://10.10.10.254/httpapi.asp?command=setPlayerCmd:play:url

		Play,
		Pause,
		Stop,
		Next,
		Previous,
		Mute,
		VolumeUp,
		VolumeDown
*/


// Multibye character set OR UNICODE  ?

class CWiimHttpClient
{
public:
	CWiimHttpClient();
	~CWiimHttpClient();

	CString m_LastStatus;

	int GetPlayerStatus(int extended=0);
	int UpdatePlayerStatusString();

	int SetBaseUrl(char *pUrl);
	char *GetEQList();
	int PlayUrl(const std::string& url);
	int ToggleMute();
	int ToggleEqualiserOnOff();

private:
	char m_LastResponse[2000];
	MEMORYSTRUCT m_data;
	WIIM_INFO m_Wiim;
	int m_CurlGlobalInit;

	int ParseJsonString();
	int HttpRequest(std::string &url);
	void CurlWiimConfig(CURL *curl_handle);
	int CurlGlobalInit_Ok();
//	int SetPlayerCmd(char *cmd, char *arg_1=nullptr, char *arg_2=nullptr);
	int SetPlayerCmd(const std::string& cmd, const std::string& arg_1="", const std::string& arg_2="");

	std::string ExtractArtworkUrl(const std::string& vendor);
	std::string FormatTimeMs(const std::string& msStr);
	std::string HexToUtf8(const std::string& hex);
	std::string UrlDecode(const std::string& src);
};
