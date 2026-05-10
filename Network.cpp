
#include "pch.h"
#include "Network.h"


CWiimHttpClient::CWiimHttpClient()
{
/*	curl_global_init()		once - (sets up: SSL backends, socket systems, global state, thread infrastructure)
	curl_easy_init()		per request
	curl_easy_cleanup()		per request
	curl_global_cleanup()	once     */
	curl_global_init(CURL_GLOBAL_ALL);

	m_data.memory = (char*)malloc(1);  // grown as needed by realloc
	m_data.size = 0; // no data at this point

	m_Wiim.Mute = m_Wiim.Volume = m_Wiim.PlayerStatus = 0;
}

CWiimHttpClient::~CWiimHttpClient()
{
	free(m_data.memory);
	// we are done with libcurl, so clean it up 
	curl_global_cleanup();
}

// To put this callback in CWiimHttpClient it must be declared as static, and to access class members/data a 'this' pointer must be passed in via the userp argument. This is a common pattern for using C-style callbacks in C++ classes.
static size_t CurlMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	MEMORYSTRUCT *mem = (MEMORYSTRUCT*)userp;

	mem->memory = (char*)realloc(mem->memory, mem->size + realsize + 1);
	if(mem->memory == NULL) {
		/* out of memory */
		TRACE("not enough memory (realloc returned NULL)\n");
		return 0;
	}

	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return realsize;
}

int CWiimHttpClient::HttpRequest(std::string &url)
{
	int Ok = 0;
	CURL *curl_handle;
	CURLcode res;

	// init the curl session
	curl_handle = curl_easy_init();

	// specify URL to get 
	curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());

	// COMMENT EACH line below...
	curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 0L);
	curl_easy_setopt(curl_handle, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
	curl_easy_setopt(curl_handle, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);
	curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 5L);
	curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1L);
	///

	/* send all data to this function  */
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, CurlMemoryCallback);

	/* we pass our m_data struct to the callback function */
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&m_data);

	/* some servers do not like requests that are made without a user-agent
	field, so we provide one */
	curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

	/* get it! */
	res = curl_easy_perform(curl_handle);

	/* check for errors */
	if(res != CURLE_OK) {
		TRACE("curl_easy_perform() failed: %s\n",
			curl_easy_strerror(res));
	}
	else {
		/* Now, our m_data.memory points to a memory block that is m_data.size bytes big and contains the remote file.
		Do something nice with it	*/

		TRACE("\n\n%lu bytes retrieved\n\n%s\n\n", (long)m_data.size, m_data.memory);
		Ok = 1;
	}

	/* cleanup curl stuff */
	curl_easy_cleanup(curl_handle);

	return Ok;
}

int CWiimHttpClient::SetPlayerCmd(char *cmd, char *arg_1, char *arg_2)
{
	std::string Url = "https://" + m_Wiim.IP + "/httpapi.asp?command=setPlayerCmd:" + cmd;
	if(arg_1 != nullptr) Url += ":" + *arg_1;
	if(arg_2 != nullptr) Url += ":" + *arg_2;
	return HttpRequest(Url);
}

/*
//	curl_easy_setopt(curl_handle, CURLOPT_URL, "https://www.example.com/");
curl_easy_setopt(curl_handle, CURLOPT_URL, "https://192.168.0.228/httpapi.asp?command=getPlayerStatus");
//	curl_easy_setopt(curl_handle, CURLOPT_URL, "https://192.168.0.228/httpapi.asp?command=setPlayerCmd:play:http://as-hls-ww-live.akamaized.net/pool_92079267/live/ww/bbc_1xtra/bbc_1xtra.isml/bbc_1xtra-audio%3d320000.norewind.m3u8");
//	curl_easy_setopt(curl_handle, CURLOPT_URL, "https://192.168.0.228/httpapi.asp?command=setPlayerCmd:mute:0");
//	curl_easy_setopt(curl_handle, CURLOPT_URL, "https://192.168.0.228/httpapi.asp?command=setPlayerCmd:play:http://listen-boomradio.sharp-stream.com/65_boom_light_256_aac?ref=RF");
*/

int CWiimHttpClient::SetBaseUrl(char *pUrl)
{
	m_Wiim.IP = std::string(pUrl);

	// here we need to get player status, to get data, eg volume, mute status, etc. and update the UI accordingly. 

	std::string Url = "https://" + m_Wiim.IP + "/httpapi.asp?command=getPlayerStatus";

	if(HttpRequest(Url)){
		// json to m_Wiim struct, then update UI with m_Wiim data.
	//	m_data.memory
		return 1;
	}
	return 0;
}

/*int CWiimHttpClient::TogglePlay()
{
	if(m_Wiim.PlayerStatus == 1) return SetPlayerCmd("pause");
	else return SetPlayerCmd("play");
}*/

int CWiimHttpClient::ReadJsonString()
{
	/* example string :
	{"type":"0","ch":"0","mode":"31","loop":"3","eq":"0","vendor":"spotify:search:Run+Up+%28feat.+PARTYNEXTDOOR+%26+Nicki+Minaj%29+Major+Lazer","status":"play",
	"curpos":"118816","offset_pts":"0","totlen":"201254","Title":"427920596F75722053696465","Artist":"4A6F6E617320426C7565","Album":"426C7565","alarmflag":"0","plicount":"0","plicurr":"0","vol":"65","mute":"0"}
	*/
	char JsonStr[] = "{\"type\":\"0\",\"ch\":\"0\",\"mode\":\"31\",\"loop\":\"3\",\"eq\":\"0\",\"vendor\":\"spotify:search:Run+Up+%28feat.+PARTYNEXTDOOR+%26+Nicki+Minaj%29+Major+Lazer\",\"status\":\"play\",\"curpos\":\"118816\",\"offset_pts\":\"0\",\"totlen\":\"201254\",\"Title\":\"427920596F75722053696465\",\"Artist\":\"4A6F6E617320426C7565\",\"Album\":\"426C7565\",\"alarmflag\":\"0\",\"plicount\":\"0\",\"plicurr\":\"0\",\"vol\":\"65\",\"mute\":\"0\"}";

	int type,ch,mode,loop,eq,alarmflag,plicount,plicurr,vol,mute;
	std::string vendor,status,curpos,offset_pts,totlen,Title,Artist,Album;

	// parse json string here, and update m_Wiim struct with the data, then update UI with m_Wiim data.



	return 1;
}

std::string CWiimHttpClient::GetPlayerStatus()
{
	std::string Url = "https://" + m_Wiim.IP + "/httpapi.asp?command=getPlayerStatus";
	if(HttpRequest(Url)){

	}
}

int CWiimHttpClient::PlayUrl()
{
	return SetPlayerCmd("play", "http://as-hls-ww-live.akamaized.net/pool_92079267/live/ww/bbc_1xtra/bbc_1xtra.isml/bbc_1xtra-audio%3d320000.norewind.m3u8");
}

int CWiimHttpClient::ToggleMute()
{
	m_Wiim.Mute = !m_Wiim.Mute;
	return SetPlayerCmd("mute", m_Wiim.Mute?"1":"0");
}


