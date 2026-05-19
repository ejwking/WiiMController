
#include "pch.h"
#include "Network.h"
#include "json.hpp"
#include "tools.h"

using json = nlohmann::json;

CWiimHttpClient::CWiimHttpClient()
{
	m_CurlErrorLog = "";
	m_ResponseErrorLog = "";
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
	RESPONSE_MEM *pMS = (RESPONSE_MEM*)userp;

	if(realsize >= pMS->mem_size){		// the +1 is for the null terminator we will add at the end of the data block. This check ensures we have enough space to store the new data plus the null terminator.
		char *pOldBuf = pMS->memory;
		pMS->mem_size = realsize + 1 + 1024; // Add some extra space, +1 for the null terminator, and the rest to reduce the number of reallocations.
		pMS->memory = (char*)realloc(pMS->memory, pMS->mem_size);		
		if(pMS->memory == NULL){
			pMS->error += "\r\n Error - Failed to allocate memory for HTTP response. The device may be sending a very large response or there may be insufficient memory available.";
			if(pOldBuf)
				free(pOldBuf);
			pMS->mem_size = 0;
			pMS->memory = NULL;
		}
	}
	if(memcpy_s(pMS->memory, pMS->mem_size, contents, realsize) != 0){
		pMS->error += "\r\n Error - Failed to copy memory from HTTP request.";
		return 0;
	}
	pMS->memory[realsize] = 0;	// null terminate the string
	pMS->response_size = realsize + 1;
	return realsize;
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

	// Set a hard timeout for the entire transfer (in seconds).
	// - CURLOPT_TIMEOUT = 2L limits the operation to 2 seconds to avoid blocking indefinitely. Tune this value based on network conditions.
	curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 2L);

	// Disable libcurl use of signals.
	// - CURLOPT_NOSIGNAL = 1L prevents libcurl from installing signal handlers (e.g. SIGALRM). This is required in multi-threaded applications and on
	//   some platforms to avoid unsafe signal interactions. It can slightly affect timeout granularity on some systems.
	curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1L);
	
/*	// Force DNS resolution to IPv4 only.
	// - CURL_IPRESOLVE_V4 ensures the request uses an IPv4 address. Useful when the target device has issues with IPv6 or when the network
	//   environment prefers IPv4.
	curl_easy_setopt(curl_handle, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);

	// Use HTTP/1.0 for the request.
	// - Some embedded or older HTTP servers require HTTP/1.0. If not required, consider using the default (HTTP/1.1 or negotiate HTTP/2) for better
	//   performance and modern features.
	curl_easy_setopt(curl_handle, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);
*/
}

int CWiimHttpClient::HttpRequest(std::string &url)
{
	int Ok = 0;
	if(CurlGlobalInit_Ok()){
		CURL *curl_handle;
		CURLcode code;
		// init the curl session
		curl_handle = curl_easy_init();
		CurlWiimConfig(curl_handle);
		// specify URL to get 
		curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
		// send all data to this function
		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, CurlMemoryCallback);
		// we pass our m_data struct to the callback function
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void*)&m_data);

		if(m_data.error != "")
			m_CurlErrorLog += m_data.error;

		// some servers do not like requests that are made without a user-agent	field, so we provide one
		curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
		// get it!
		code = curl_easy_perform(curl_handle);
		// check for errors
		if(code != CURLE_OK){
			m_CurlErrorLog += "\r\n Error - curl_easy_perform() failed: ";
			m_CurlErrorLog += curl_easy_strerror(code);
			m_CurlErrorLog += "\r\n ..( device is offline or IP address is incorrect )";
		}
		else{
			// Now, our m_data.memory points to a memory block that is m_data.response_size bytes big and contains the remote file. Do something nice with it.
			TRACE("\n\n%d bytes retrieved\n%s\n\n", m_data.response_size, m_data.memory);
			Ok = 1;
		}
		// cleanup curl stuff
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
		m_CurlGlobalInit = 1;
	}

	if(m_data.memory==NULL && m_CurlErrorLog==""){	// m_CurlErrorLog=="" ensures there will only be one attempt, to allocate memory for the response.
		m_data.mem_size = 10000;	// initial size, will be grown as needed by realloc in the callback.
		m_data.memory = (char*)malloc(m_data.mem_size);
		m_data.error = "";
		if(m_data.memory == NULL){
			m_CurlErrorLog += "\r\n Error - Failed to allocate memory for CURL response";
			m_data.mem_size = 0;
		}
	}
	return (m_CurlGlobalInit && m_data.memory!=NULL);
}

int CWiimHttpClient::SendCommand(const std::string& command, const std::string& arg_1, const std::string& arg_2, const std::string& arg_3)
{
	// example - setPlayerCmd:play:url
	//           command:arg_1:arg_2
	if(CurlGlobalInit_Ok()){
		std::string Url = "https://" + m_Wiim.IP + "/httpapi.asp?command=" + command;
		if(arg_1 != "") Url += ":" + arg_1;
		if(arg_2 != "") Url += ":" + arg_2;
		if(arg_3 != "") Url += ":" + arg_3;
		if(HttpRequest(Url))
			return 1;
	}
	return 0;
}

int CWiimHttpClient::SetWiimIPaddress(BYTE Field0, BYTE Field1, BYTE Field2, BYTE Field3)
{
	m_Wiim.IP = std::to_string(Field0) + "." + std::to_string(Field1) + "." + std::to_string(Field2) + "." + std::to_string(Field3);
	return 1;
}

int CWiimHttpClient::GetStatusEx()
{
	// getStatusEx this retrieves masses of info.
	if(SendCommand("getStatusEx")){
		// partial example response string:
		// { "language": "en_us", "ssid": "WiiM Pro-932C", "hideSSID": "0", "firmware": "Linkplay.4.8.814756", "build": "release", "project": "WiiM_Pro_with_gc4a", "priv_prj": "WiiM_Pro_with_gc4a", "project_build_name": "WiiM_Pro_with_gc4a", "Release": "20260423", 
		// "FW_Release_version": "", "PCB_version": "1", "cast_enable": 1, "cast_usage_report": 0, "group": "0", "wmrm_version": "4.3", "wmrm_sub_ver": "7", "wmrm_capability": 63, "expired": "0", "internet": "1", "uuid": 

	//	if(ParseJsonString_StatusEx())
	//		return 1;
	}
	return 0;
}

int CWiimHttpClient::GetMetaInfo()
{
	if(SendCommand("getMetaInfo")){
		// example response - { "metaData": { "album": "unknow", "title": "https://streaming05.liveboxstream.uk/proxy/selectr1/stream", "subtitle": "unknow", "artist": "unknow", "albumArtURI": "unknow", "sampleRate": "44100", "bitDepth": "32", "bitRate": "127", "trackId": "0" } } */
		if(strcmp(m_data.memory, "Failed") != 0)
			if(ParseJsonString_MetaInfo())
				return 1;
	}
	return 0;
}

void CWiimHttpClient::ResetMetaInfo()
{
	m_Wiim.sampleRate = "";
	m_Wiim.bitDepth = "";
	m_Wiim.bitRate = "";
}

int CWiimHttpClient::GetPlayerStatus()
{
	if(SendCommand("getPlayerStatus")){
		// getPlayerStatus example string:
		// {"type":"0","ch":"0","mode":"31","loop":"3","eq":"0","vendor":"spotify:search:Run+Up+%28feat.+PARTYNEXTDOOR+%26+Nicki+Minaj%29+Major+Lazer","status":"play","curpos":"118816","offset_pts":"0",
		// "totlen":"201254","Title":"427920596F75722053696465","Artist":"4A6F6E617320426C7565","Album":"426C7565","alarmflag":"0","plicount":"0","plicurr":"0","vol":"65","mute":"0"}
		if(ParseJsonString_PlayerStatus())
			return 1;
	}
	return 0;
}

int CWiimHttpClient::PlayUrl(const std::string& url)
{
//	if(SendCommand("setPlayerCmd", "playlist", url, "0")){	this command can be used instead of below.
	if(SendCommand("setPlayerCmd", "play", url)){
		m_Wiim.Title = url;
		return 1;
	}
	return 0;
}

int CWiimHttpClient::GetEqStatus(int &EqualiserOn)
{
	// example response for EQGetBand:
	// {"status":"OK","EQLevel":1,"source_name":"wifi","EQStat":"Off","Name":"bass up treble down","pluginURI":"http://moddevices.com/plugins/caps/Eq10HP","channelMode":"Stereo","EQBand":[{"index":0,"param_name":"band31hz","value":100},{"index":1,"param_name":"band63hz","value":100},{"index":2,"param_name":"band125hz","value":80},{"index":3,"param_name":"band250hz","value":55},{"index":4,"param_name":"band500hz","value":50},{"index":5,"param_name":"band1khz","value":50},{"index":6,"param_name":"band2khz","value":50},{"index":7,"param_name":"band4khz","value":56},{"index":8,"param_name":"band8khz","value":71},{"index":9,"param_name":"band16khz","value":86}]}
	int Ok = 0;
	if(SendCommand("EQGetBand")){
		if(ParseJsonString_EqBand()){
			m_EqualiserOn = -1; // Unknown state
				 if(m_Eq.EQStat.compare("On")  == 0) m_EqualiserOn = 1;
			else if(m_Eq.EQStat.compare("Off") == 0) m_EqualiserOn = 0;
			Ok = 1;
			EqualiserOn = m_EqualiserOn;
		}
	}
	return Ok;
}

int CWiimHttpClient::GetError(CString &ErrorString)
{
	// check the error string is the same length as it was last time we checked, if its not then new error should be displayed in UI.
	static size_t PrevErrorLength = 0;
	size_t ErrorLength = m_CurlErrorLog.length() + m_ResponseErrorLog.length();
	if(ErrorLength != PrevErrorLength){
		PrevErrorLength = ErrorLength;
		ErrorString = Utf8(m_CurlErrorLog + m_ResponseErrorLog);
		return 1;
	}
	return 0;
}

int CWiimHttpClient::EQLoad(char *pName)
{
	// SendCommand("EQLoad", "Classical");
	if(SendCommand("EQLoad", pName)){
		m_EqualiserOn = 1;
		return 1;
	}
	return 0;
}

int CWiimHttpClient::ToggleEqualiserOnOff()
{
	m_EqualiserOn = !m_EqualiserOn;
	SendCommand(m_EqualiserOn ? "EQOn" : "EQOff");
	return m_EqualiserOn;
}

char *CWiimHttpClient::GetEQList()
{
	// EQGetBand / EQSetBand, command not in pdf, see wiim forum
	if(SendCommand("EQGetList")){
		// Example Response:
		// [ "Flat", "Acoustic", "Bass Booster", "Bass Reducer", "Classical", "Dance", "Deep", "Electronic", "Game", "Hip-Hop", "Jazz", "Latin", "Loudness", "Lounge", "Movie", "Piano", "Pop", "R&B", "Rock", "Small Speakers", "Spoken Word", "Treble Booster", "Treble Reducer", "Vocal Booster", "bass up treble down", "low frequencys up", "middle down", "middle down 2" ]
		return m_data.memory;
	}
	return NULL;
}

int CWiimHttpClient::TogglePlay()
{
	// 2.3.6 Toggle pause/play, Params: setPlayerCmd:onepause
	// If the state is paused, resume it; otherwise, pause it.
	m_Wiim.status = (m_Wiim.status == "play") ? "pause" : "play";
	return SendCommand("setPlayerCmd", "onepause");

	// or use this instead...
	// play / pause / stop / resume
/*	if(m_Wiim.status.compare("play") == 0)
		return SendCommand("setPlayerCmd", "pause");
	return SendCommand("setPlayerCmd", "play"); */
}

int CWiimHttpClient::ToggleMute()
{
	m_Wiim.mute = !m_Wiim.mute;
	return SendCommand("setPlayerCmd", "mute", m_Wiim.mute?"1":"0");
}

int CWiimHttpClient::VolumeStep(int step)
{
	// 2.3.11 Set volume, Params: setPlayerCmd:vol:value
	// https://10.10.10.254/httpapi.asp?command=setPlayerCmd:vol:value
	// Value can be 0 to 100.
	m_Wiim.vol += step;
	m_Wiim.vol = min(m_Wiim.vol, 100);
	m_Wiim.vol = max(m_Wiim.vol,   0);
	SendCommand("setPlayerCmd", "vol", std::to_string(m_Wiim.vol));
	m_Wiim.mute = 0;
	return 0;
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

int CWiimHttpClient::ParseJsonString_PlayerStatus()
{
	json j;
	try {
		j = json::parse(m_data.memory);
	}
	catch (...) {
		m_ResponseErrorLog += "\r\n PlayerStatus. Failed to parse JSON response. The response may be malformed or the device may not be responding correctly.";
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
	if(m_Wiim.Artist == "")	m_Wiim.Artist = "Unknown";
	if(m_Wiim.Album  == "")	m_Wiim.Album  = "Unknown";
	// Time formatting
	m_Wiim.curpos_fmt = FormatTimeMs(m_Wiim.curpos);
	m_Wiim.totlen_fmt = FormatTimeMs(m_Wiim.totlen);
	// Artwork URL (if present)
	m_Wiim.ArtUrl = ExtractArtworkUrl(m_Wiim.vendor);
	return 1;
}

int CWiimHttpClient::ParseJsonString_EqBand()
{
	json j;
	try {
		j = json::parse(m_data.memory);
	}
	catch (...) {
		m_ResponseErrorLog += "\r\n EqBand. Failed to parse JSON response. The response may be malformed or the device may not be responding correctly.";
		return 0;
	}
	m_Eq.status      = j.value("status", "");
	m_Eq.source_name = j.value("source_name", "");
	m_Eq.EQStat      = j.value("EQStat", "");
	m_Eq.Name        = j.value("Name", "");
	m_Eq.pluginURI   = j.value("pluginURI", "");
	m_Eq.channelMode = j.value("channelMode", "");

	m_Eq.EQLevel = j.value("EQLevel", 0);
	return 1;
}

int CWiimHttpClient::ParseJsonString_MetaInfo()
{
	json j;
	try {
		j = json::parse(m_data.memory);
	}
	catch (...) {
		m_ResponseErrorLog += "\r\n MetaInfo. Failed to parse JSON response. The response may be malformed or the device may not be responding correctly.";
		return 0;
	}
	// metaData is nested in the response: { "metaData": { "sampleRate": "...", "bitDepth": "...", "bitRate": "..." } }
	if (j.contains("metaData") && j["metaData"].is_object()) {
		auto &md = j["metaData"];
		m_Wiim.sampleRate = md.value("sampleRate", "");
		m_Wiim.bitDepth   = md.value("bitDepth", "");
		m_Wiim.bitRate    = md.value("bitRate", "");
	}
	else{
		// Backwards-compatible fallback if device ever returns top-level fields
		m_Wiim.sampleRate = j.value("sampleRate", "");
		m_Wiim.bitDepth   = j.value("bitDepth", "");
		m_Wiim.bitRate    = j.value("bitRate", "");
	}
	return 1;
}
