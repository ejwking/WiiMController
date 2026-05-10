
#include "pch.h"
#include "Network.h"
#include "json.hpp"

using json = nlohmann::json;

CWiimHttpClient::CWiimHttpClient()
{
/*	curl_global_init()		once - (sets up: SSL backends, socket systems, global state, thread infrastructure)
	curl_easy_init()		per request
	curl_easy_cleanup()		per request
	curl_global_cleanup()	once     */
	curl_global_init(CURL_GLOBAL_ALL);

	m_data.memory = (char*)malloc(1);  // grown as needed by realloc
	m_data.size = 0; // no data at this point

	m_Init = 0;
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

	/* some servers do not like requests that are made without a user-agent	field, so we provide one */
	curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

	/* get it! */
	res = curl_easy_perform(curl_handle);

	/* check for errors */
	if(res != CURLE_OK) {
		TRACE("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
	}
	else {
		// Now, our m_data.memory points to a memory block that is m_data.size bytes big and contains the remote file. Do something nice with it.
		TRACE("\n\n%lu bytes retrieved\n\n%s\n\n", (long)m_data.size, m_data.memory);
		Ok = 1;
	}

	/* cleanup curl stuff */
	curl_easy_cleanup(curl_handle);
	return Ok;
}

int CWiimHttpClient::SetPlayerCmd(char *cmd, char *arg_1, char *arg_2)
{
	if(m_Init){
		std::string Url = "https://" + m_Wiim.IP + "/httpapi.asp?command=setPlayerCmd:" + cmd;

		not working for mute..
		if(arg_1 != nullptr) Url += ":" + *arg_1;
		if(arg_2 != nullptr) Url += ":" + *arg_2;
		return HttpRequest(Url);
	}
	return 0;
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
	return GetPlayerStatus();
}

/*int CWiimHttpClient::TogglePlay()
{
	if(m_Wiim.PlayerStatus == 1) return SetPlayerCmd("pause");
	else return SetPlayerCmd("play");
}*/

int CWiimHttpClient::GetPlayerStatus()
{
	// Use getPlayerStatusEx for a lot more info.
	//
	// getPlayerStatus example string:
	// {\"type\":\"0\",\"ch\":\"0\",\"mode\":\"31\",\"loop\":\"3\",\"eq\":\"0\",\"vendor\":\"spotify:search:Run+Up+%28feat.+PARTYNEXTDOOR+%26+Nicki+Minaj%29+Major+Lazer\",\"status\":\"play\",\"curpos\":\"118816\",\"offset_pts\":\"0\",\"totlen\":\"201254\",\"Title\":\"427920596F75722053696465\",\"Artist\":\"4A6F6E617320426C7565\",\"Album\":\"426C7565\",\"alarmflag\":\"0\",\"plicount\":\"0\",\"plicurr\":\"0\",\"vol\":\"65\",\"mute\":\"0\"}

	std::string Url = "https://" + m_Wiim.IP + "/httpapi.asp?command=getPlayerStatus";
	if(HttpRequest(Url)){
		if(ParseJsonString()){
			m_Init = 1;
			return 1;
		}
	}
	m_Init = 0;
	return 0;
}

int CWiimHttpClient::PlayUrl()
{
	return SetPlayerCmd("play", "http://as-hls-ww-live.akamaized.net/pool_92079267/live/ww/bbc_1xtra/bbc_1xtra.isml/bbc_1xtra-audio%3d320000.norewind.m3u8");
}

int CWiimHttpClient::ToggleMute()
{
	m_Wiim.mute = !m_Wiim.mute;
	return SetPlayerCmd("mute", m_Wiim.mute?"1":"0");
}

std::string CWiimHttpClient::ExtractArtworkUrl(const std::string& vendor)
{
	const std::string key = "artwork_url=";
	size_t pos = vendor.find(key);
	if (pos == std::string::npos)
		return "";

	std::string encoded = vendor.substr(pos + key.length());
	return UrlDecode(encoded);
}

std::string CWiimHttpClient::FormatTimeMs(const std::string& msStr)
{
	long ms = std::stol(msStr);
	long totalSec = ms / 1000;
	int minutes = totalSec / 60;
	int seconds = totalSec % 60;
	char buf[16];
	snprintf(buf, sizeof(buf), "%d:%02d", minutes, seconds);
	return std::string(buf);
}

std::string CWiimHttpClient::HexToUtf8(const std::string& hex)
{
	std::string out;
	out.reserve(hex.size() / 2);
	for (size_t i = 0; i + 1 < hex.size(); i += 2)
	{
		unsigned char byte = static_cast<unsigned char>(strtol(hex.substr(i, 2).c_str(), nullptr, 16));
		out.push_back(byte);
	}
	return out; // already valid UTF‑8
}

std::string CWiimHttpClient::UrlDecode(const std::string& src)
{
	std::string out;
	out.reserve(src.size());

	for(size_t i=0; i<src.size(); ++i){
		if (src[i] == '+')
			out.push_back(' ');
		else if (src[i] == '%' && i + 2 < src.size()){
			std::string hex = src.substr(i + 1, 2);
			char chr = static_cast<char>(strtol(hex.c_str(), nullptr, 16));
			out.push_back(chr);
			i += 2;
		}
		else
			out.push_back(src[i]);
	}
	return out;
}

int CWiimHttpClient::ParseJsonString()
{
	json j;
	try {
		j = json::parse(m_data.memory);
	}
	catch (...) {
		return 0;
	}

	// Integer fields
	m_Wiim.type      = std::stoi(j.value("type", "0"));
	m_Wiim.ch        = std::stoi(j.value("ch", "0"));
	m_Wiim.mode      = std::stoi(j.value("mode", "0"));
	m_Wiim.loop      = std::stoi(j.value("loop", "0"));
	m_Wiim.eq        = std::stoi(j.value("eq", "0"));
	m_Wiim.alarmflag = std::stoi(j.value("alarmflag", "0"));
	m_Wiim.plicount  = std::stoi(j.value("plicount", "0"));
	m_Wiim.plicurr   = std::stoi(j.value("plicurr", "0"));
	m_Wiim.vol       = std::stoi(j.value("vol", "0"));
	m_Wiim.mute      = std::stoi(j.value("mute", "0"));

	// Strings
	m_Wiim.vendor     = UrlDecode(j.value("vendor", ""));
	m_Wiim.status     = j.value("status", "");
	m_Wiim.curpos     = j.value("curpos", "");
	m_Wiim.offset_pts = j.value("offset_pts", "");
	m_Wiim.totlen     = j.value("totlen", "");

	// Hex → UTF‑8
	m_Wiim.Title  = HexToUtf8(j.value("Title", ""));
	m_Wiim.Artist = HexToUtf8(j.value("Artist", ""));
	m_Wiim.Album  = HexToUtf8(j.value("Album", ""));

	// Time formatting
	m_Wiim.curpos_fmt = FormatTimeMs(m_Wiim.curpos);
	m_Wiim.totlen_fmt = FormatTimeMs(m_Wiim.totlen);

	// Artwork URL (if present)
	m_Wiim.ArtUrl = ExtractArtworkUrl(m_Wiim.vendor);

	return 1;
}

/*	// Hex → ASCII
	auto HexToAscii = [](const std::string& hex) {
		std::string out;
		for (size_t i = 0; i < hex.length(); i += 2) {
			char chr = (char) strtol(hex.substr(i, 2).c_str(), nullptr, 16);
			out.push_back(chr);
		}
		return out;
		};

	m_Wiim.Title  = HexToAscii(j.value("Title", ""));
	m_Wiim.Artist = HexToAscii(j.value("Artist", ""));
	m_Wiim.Album  = HexToAscii(j.value("Album", ""));
	*/
