

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
miniupnpc, simple, widely used
Write your own SSDP client (UDP multicast to 239.255.255.250:1900)

You can send 'HTTPs Get' request to the device, most of the response is in the JSON format.
Request format is https://x.x.x.x/httpapi.asp?command=********
x.x.x.x is the IP address of the device, ******* is the actual command.

*/

struct WIIM_STATUS
{
	// getStatusEx response fields
	std::string Name, IP, Model, SerialNumber;

	// getPlayerStatus response fields
	std::string vendor, status, offset_pts;
	std::string Title, Artist, Album, ArtUrl;
	std::string curpos, curpos_fmt;
	std::string totlen, totlen_fmt;

	int type = 0, ch = 0, mode = 0, loop = 0, eq = 0;
	int alarmflag = 0, plicount = 0, plicurr = 0, vol = 0, mute = 0;

	// getMetaInfo response fields
	std::string sampleRate, bitDepth, bitRate;
};

struct WIIM_EQUALISER
{
	std::string status, source_name, EQStat, Name, pluginURI, channelMode;
	int EQLevel;
};

struct RESPONSE_MEM
{
	char *memory;
	size_t mem_size;
	size_t response_size;
	std::string error;
};

class CWiimHttpClient
{
public:
	CWiimHttpClient();
	~CWiimHttpClient();

	WIIM_STATUS m_Wiim;
	WIIM_EQUALISER m_Eq;
	std::string m_CurlErrorLog, m_ResponseErrorLog;

	int GetStatusEx();
	int GetMetaInfo();
	void ResetMetaInfo();
	int GetPlayerStatus();
	int SetBaseUrl(char *pUrl);
	char *GetEQList();
	int TogglePlay();
	int PlayUrl(const std::string& url);
	int GetEqStatus(int &EqualiserOn);
	int GetError(CString &ErrorString);
	int EQLoad(char *pName);
	int ToggleMute();
	int VolumeStep(int step); // positive or negative
	int ToggleEqualiserOnOff();

private:
	RESPONSE_MEM m_data;

	int m_CurlGlobalInit;
	int m_EqualiserOn = 0;

	int ParseJsonString_PlayerStatus();
	int ParseJsonString_EqBand();
	int ParseJsonString_MetaInfo();
	int HttpRequest(std::string &url);
	void CurlWiimConfig(CURL *curl_handle);
	int CurlGlobalInit_Ok();
	int SendCommand(const std::string& command, const std::string& arg_1="", const std::string& arg_2="", const std::string& arg_3="");

	std::string ExtractArtworkUrl(const std::string& vendor);
	std::string FormatTimeMs(const std::string& msStr);
	std::string UrlDecode(const std::string& src);
};
