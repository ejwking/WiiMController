
#include "pch.h"
#include "Network.h"
#include "json.hpp"
#include "tools.h"

using json = nlohmann::json;

CWiimHttpClient::CWiimHttpClient()
{
	m_CurlGlobalInit = 0;
	memset(&m_data, 0, sizeof(m_data));
}

CWiimHttpClient::~CWiimHttpClient()
{
	if(m_CurlGlobalInit){
		free(m_data.memory);
		// we are done with libcurl, so clean it up 
		curl_global_cleanup();
		m_CurlGlobalInit = 0;
	}
}

// To put this callback in CWiimHttpClient it must be declared as static, and to access class members/data a 'this' pointer must be passed in via the userp argument. This is a common pattern for using C-style callbacks in C++ classes.
static size_t CurlMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	MEMORYSTRUCT *pMS = (MEMORYSTRUCT*)userp;

	if(realsize+1 > pMS->mem_size){		// the +1 is for the null terminator we will add at the end of the data block. This check ensures we have enough space to store the new data plus the null terminator.
		char *pOldBuf = pMS->memory;
		pMS->memory = (char*)realloc(pMS->memory, realsize+1);
		pMS->mem_size = realsize+1;
		
		if(pMS->memory == NULL){
			AfxMessageBox(_T("Failed to allocate memory for HTTP response. The device may be sending a very large response or there may be insufficient memory available."));
			if(pOldBuf)
				free(pOldBuf);
			pMS->mem_size = 0;
			return 0;
		}
	}
	if(memcpy_s(pMS->memory, pMS->mem_size, contents, realsize) != 0){
		AfxMessageBox(_T("Failed to copy memory for HTTP response."));
		return 0;
	}
	pMS->memory[realsize] = 0;	// null terminate the string
	pMS->response_size = realsize+1;
	return realsize;

	/*	this original code adds the new data to the end of the existing data in pMS->memory, but i dont need to keep the old data, so i can just overwrite it each time, and only allocate enough memory for the new data.
	pMS->memory = (char*)realloc(pMS->memory, pMS->mem_size + realsize + 1);
	if(pMS->memory == NULL) {
		// out of memory
		TRACE("not enough memory (realloc returned NULL)\n");
		return 0;
	}
	memcpy(&(pMS->memory[pMS->mem_size]), contents, realsize);
	pMS->mem_size += realsize;
	pMS->memory[pMS->mem_size] = 0;
	return realsize;*/
}

void CWiimHttpClient::CurlWiimConfig(CURL *curl_handle)
{
	// Specific setup to get the request working with the Wiim device. (Adjust these options based on your needs and security requirements).

	// Disable verification of the peer's SSL certificate.
	// - CURLOPT_SSL_VERIFYPEER == 0L turns off checking that the server's certificate is signed by a trusted CA. This is insecure for public
	//   networks but is sometimes used for local devices with self-signed certificates. For production clients prefer 1L and provide a CA bundle.
	curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0L);

	// Disable verification that the server hostname matches the certificate.
	// - CURLOPT_SSL_VERIFYHOST == 0L disables hostname checking. This further weakens TLS security. Use 2L to enable hostname verification.
	curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 0L);

	// Force DNS resolution to IPv4 only.
	// - CURL_IPRESOLVE_V4 ensures the request uses an IPv4 address. Useful when the target device has issues with IPv6 or when the network
	//   environment prefers IPv4.
	curl_easy_setopt(curl_handle, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);

	// Use HTTP/1.0 for the request.
	// - Some embedded or older HTTP servers require HTTP/1.0. If not required, consider using the default (HTTP/1.1 or negotiate HTTP/2) for better
	//   performance and modern features.
	curl_easy_setopt(curl_handle, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);

	// Set a hard timeout for the entire transfer (in seconds).
	// - CURLOPT_TIMEOUT = 5L limits the operation to 5 seconds to avoid blocking indefinitely. Tune this value based on network conditions.
	curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 5L);

	// Disable libcurl use of signals.
	// - CURLOPT_NOSIGNAL = 1L prevents libcurl from installing signal handlers (e.g. SIGALRM). This is required in multi-threaded applications and on
	//   some platforms to avoid unsafe signal interactions. It can slightly affect timeout granularity on some systems.
	curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1L);
}

int CWiimHttpClient::HttpRequest(std::string &url)
{
	int Ok = 0;
	if(CurlGlobalInit_Ok()){
		CURL *curl_handle;
		CURLcode result;
		// init the curl session
		curl_handle = curl_easy_init();
		CurlWiimConfig(curl_handle);
		// specify URL to get 
		curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
		/* send all data to this function  */
		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, CurlMemoryCallback);
		/* we pass our m_data struct to the callback function */
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void*)&m_data);
		/* some servers do not like requests that are made without a user-agent	field, so we provide one */
		curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
		/* get it! */
		result = curl_easy_perform(curl_handle);
		/* check for errors */
		if(result != CURLE_OK){
			std::string err = "curl_easy_perform() failed: \n\n";
			err += curl_easy_strerror(result);
			AfxMessageBox(Utf8(err));
		}
		else{
			// Now, our m_data.memory points to a memory block that is m_data.response_size bytes big and contains the remote file. Do something nice with it.
			TRACE("\n\n%d bytes retrieved\n%s\n\n", m_data.response_size, m_data.memory);
			Ok = 1;
		}
		/* cleanup curl stuff */
		curl_easy_cleanup(curl_handle);
	}
	return Ok;
}

int CWiimHttpClient::CurlGlobalInit_Ok()
{
	if(!m_CurlGlobalInit){
	/*	curl_global_init()		once - (sets up: SSL backends, socket systems, global state, thread infrastructure)
		curl_easy_init()		per request
		curl_easy_cleanup()		per request
		curl_global_cleanup()	once     */

		curl_global_init(CURL_GLOBAL_ALL);
		m_data.mem_size = 10000; // initial size, will be grown as needed by realloc in the callback
		m_data.memory = (char*)malloc(m_data.mem_size);
		if(m_data.memory == NULL){
			AfxMessageBox(_T("Failed to allocate memory for CURL response\n"));
			m_data.mem_size = 0;
			return 0;
		}
		m_CurlGlobalInit = 1;
	}
	return m_CurlGlobalInit;
}

int CWiimHttpClient::SetPlayerCmd(const std::string& cmd, const std::string& arg_1, const std::string& arg_2)
{
	if(CurlGlobalInit_Ok()){
		std::string Url = "https://" + m_Wiim.IP + "/httpapi.asp?command=setPlayerCmd:" + cmd;
		if(arg_1 != "")
			Url += ":" + arg_1;
		if(arg_2 != "")
			Url += ":" + arg_2;
		if(HttpRequest(Url)){
			// copy the response into m_LastResponse because m_data.memory will be overwritten by the next request for the status update.
			if(sizeof(m_LastResponse) > m_data.response_size){
				memcpy_s(m_LastResponse, sizeof(m_LastResponse), m_data.memory, m_data.response_size);
				if(GetPlayerStatus())	// update our status info after sending any command, so we can reflect any info changes in the UI.
					return 1;
			}
			else
				AfxMessageBox(_T("m_LastResponse buffer to small - todo, allocate this correctly"));
		}
	}
	return 0;
}

int CWiimHttpClient::SetBaseUrl(char *pUrl)
{
	// temp until discovery is implemented, just set the IP address of the device here.
	m_Wiim.IP = std::string(pUrl);
	return 1;
}

/*int CWiimHttpClient::TogglePlay()
{
	if(m_Wiim.PlayerStatus == 1) return SetPlayerCmd("pause");
	else return SetPlayerCmd("play");
}*/

int CWiimHttpClient::GetPlayerStatus(int extended)
{
	// getPlayerStatus example string (Use getPlayerStatusEx for a lot more info) :
	// {\"type\":\"0\",\"ch\":\"0\",\"mode\":\"31\",\"loop\":\"3\",\"eq\":\"0\",\"vendor\":\"spotify:search:Run+Up+%28feat.+PARTYNEXTDOOR+%26+Nicki+Minaj%29+Major+Lazer\",\"status\":\"play\",\"curpos\":\"118816\",\"offset_pts\":\"0\",\"totlen\":\"201254\",\"Title\":\"427920596F75722053696465\",\"Artist\":\"4A6F6E617320426C7565\",\"Album\":\"426C7565\",\"alarmflag\":\"0\",\"plicount\":\"0\",\"plicurr\":\"0\",\"vol\":\"65\",\"mute\":\"0\"}
	std::string Url;
	if(extended) Url = "https://" + m_Wiim.IP + "/httpapi.asp?command=getStatusEx";
	else         Url = "https://" + m_Wiim.IP + "/httpapi.asp?command=getPlayerStatus";
	if(HttpRequest(Url)){
		if(ParseJsonString()){
			m_Wiim.Initialised = 1;
			UpdatePlayerStatusString();
			return 1;
		}
	}
	m_Wiim.Initialised = 0;
	return 0;
}

int CWiimHttpClient::UpdatePlayerStatusString()
{
	m_LastStatus.Empty();
	if(m_Wiim.Initialised){
		std::string title = (m_Wiim.vendor == "CustomPushUrl") ? "Custom URL" : m_Wiim.Title;
		m_LastStatus.Format(_T("Title: %s\r\nArtist: %s\r\nAlbum: %s\r\nPosition: %s / %s\r\nStatus: %s\r\nPlaylist: %d / %d\r\nVolume: %d\r\nMute: %d"),
			Utf8(title),
			Utf8(m_Wiim.Artist),
			Utf8(m_Wiim.Album),
			Utf8(m_Wiim.curpos_fmt),
			Utf8(m_Wiim.totlen_fmt),
			Utf8(m_Wiim.status),
			m_Wiim.plicurr, m_Wiim.plicount,
			m_Wiim.vol, m_Wiim.mute
		);
		return 1;
	}
	return 0;
}

int CWiimHttpClient::PlayUrl(const std::string& url)
{
	return SetPlayerCmd("play", url);
}

char *CWiimHttpClient::GetEQList()
{

	// EQLoad:XXXX   works in browser !!! 
	why not here.
	// then everything else / EQGetList etc works.
	// but why

	SetPlayerCmd("EQLoad", "Classical");

	// EQGetList does not work !
	// nor does EQGetStat.
	
	SetPlayerCmd("EQGetBand"); // command not in pdf, see wiim forum
	// EQGetBand / EQSetBand
	
//	if(SetPlayerCmd("EQLoad", "Flat")){
//	if(SetPlayerCmd("EQGetStat")){
	if(SetPlayerCmd("EQGetList")){
		// Example Response:
		// ["Flat", "Acoustic", "Bass Booster", "Bass Reducer", "Classical", "Dance", "Deep", "Electronic", "Hip-Hop", "Jazz", "Latin", "Loudness", "Lounge", "Piano", "Pop", "R&B", "Rock", "Small Speakers", "Spoken Word", "Treble Booster", "Treble Reducer", "Vocal Booster"]
		return m_LastResponse;
	}
	return NULL;
}

int CWiimHttpClient::ToggleMute()
{
	m_Wiim.mute = !m_Wiim.mute;
	return SetPlayerCmd("mute", m_Wiim.mute?"1":"0");
}

int CWiimHttpClient::ToggleEqualiserOnOff()
{
	m_Wiim.EqualiserOn = !m_Wiim.EqualiserOn;
	SetPlayerCmd(m_Wiim.EqualiserOn?"EQOn":"EQOff");
	return m_Wiim.EqualiserOn;
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
		AfxMessageBox(_T("Failed to parse JSON response. The response may be malformed or the device may not be responding correctly."));
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
