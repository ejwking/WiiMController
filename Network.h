

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


struct WIIM_DEVICE
{
	std::string Name, IP, Model, SerialNumber;

	int PlayerStatus; // 0 = Stopped, 1 = Playing, 2 = Paused
	int Mute, Volume;
};

struct MEMORYSTRUCT
{
	char *memory;
	size_t size;
};

/*
You can send 'HTTPs Get' request to the device, most of the response is in the JSON format.
Request format is https://x.x.x.x/httpapi.asp?command=********
x.x.x.x is the IP address of the device, ******* is the actual command.

https://10.10.10.254/httpapi.asp?command=setPlayerCmd:play:url
*/

/*
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

	std::string GetPlayerStatus();
	int SetBaseUrl(char *pUrl);
	int PlayUrl();
	int ToggleMute();

private:
	MEMORYSTRUCT m_data;
	WIIM_DEVICE m_Wiim;
	int m_Init;

	int ReadJsonString();
	int HttpRequest(std::string &url);

	int SetPlayerCmd(char *cmd, char *arg_1=nullptr, char *arg_2=nullptr);

	std::string HexToString(const std::string& hex);
};
