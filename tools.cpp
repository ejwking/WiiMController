
#include "pch.h"
#include "tools.h"


int LoadEditboxFont(CWnd *pEditbox)
{
	// change the text size.
	// https://learn.microsoft.com/en-us/answers/questions/1307887/how-to-set-font-for-single-control-in-dialog

	/*	// this doesnt work properly, 
	// lf.lfFaceName doesnt do anything, nor does changing lfHeight
	LOGFONT lf{};
	CFont NewFont, *pFont = pEditbox->GetFont(); // Get initial font
	pFont->GetLogFont(&lf); // Use LOGFONT to get font information
	lf.lfHeight = 120; // Use to create 12 point font
	//	lf.lfFaceName = "Courier"
	NewFont.CreatePointFontIndirect(&lf); // Create a 12 point font
	pEditbox->SetFont(&NewFont, 0); // Have edit control use the 12 point font
	*/

	CFont m_font;
	int Point = 16;
	// wont let me change the font, size just seems to change space between lines, oh well its an improvement anyway
	m_font.CreateFont(Point,0,0,0,100,FALSE,FALSE,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,FF_SWISS,_T("Courier"));
	pEditbox->SetFont(&m_font,FALSE);
	m_font.DeleteObject();
	return Point;
}

inline CString Utf8(const std::string& s)
{
	return CString(CA2W(s.c_str(), CP_UTF8));
}

/*
This HexToUtf8 function was about 10 lines long, but I had to add a lot of code to handle the HTML entity decoding (thanks to Copilot), which is needed because the device 
sends some characters as HTML entities in the Title and Artist fields, for example the ' character is sent as &apos; and needs to be converted back to '.

hex to Utf8 conversion problem example in the original version (that is now fixed):  
"Title":"4461766964202844617272656E20456D6572736F6E2661706F733B7320556E6465727761746572204D697829","Artist":"4E6F7720506C6179696E673A2047757320477573",
Title: David (Darren Emerson&apos;s Underwater Mix)
Artist: Now Playing: Gus Gus
... the title hasnt been converted correctly, the &apos; is still in there instead of the ' character, 
*/
std::string HexToUtf8(const std::string& hex)
{
	std::string out;
	out.reserve(hex.size() / 2);

	auto hexVal = [](char c)->int{
		if (c >= '0' && c <= '9') return c - '0';
		if (c >= 'a' && c <= 'f') return c - 'a' + 10;
		if (c >= 'A' && c <= 'F') return c - 'A' + 10;
		return -1;
		};

	// Convert hex pairs to raw bytes
	for (size_t i = 0; i + 1 < hex.size(); i += 2) {
		int hi = hexVal(hex[i]);
		int lo = hexVal(hex[i + 1]);
		if (hi < 0 || lo < 0) {
			// Skip invalid nibble pairs
			continue;
		}
		unsigned char byte = static_cast<unsigned char>((hi << 4) | lo);
		out.push_back(static_cast<char>(byte));
	}

	// Helper to append a Unicode codepoint as UTF-8
	auto appendCodepointUtf8 = [](std::string &dst, unsigned int cp) {
		if (cp <= 0x7F) {
			dst.push_back(static_cast<char>(cp));
		}
		else if (cp <= 0x7FF) {
			dst.push_back(static_cast<char>(0xC0 | ((cp >> 6) & 0x1F)));
			dst.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
		}
		else if (cp <= 0xFFFF) {
			dst.push_back(static_cast<char>(0xE0 | ((cp >> 12) & 0x0F)));
			dst.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
			dst.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
		}
		else {
			dst.push_back(static_cast<char>(0xF0 | ((cp >> 18) & 0x07)));
			dst.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
			dst.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
			dst.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
		}
		};

	// HTML entity decode (handles common named entities and numeric entities)
	std::string decoded;
	decoded.reserve(out.size());
	for (size_t i = 0; i < out.size(); ) {
		if (out[i] == '&') {
			size_t sem = out.find(';', i + 1);
			if (sem == std::string::npos) {
				decoded.push_back(out[i++]);
				continue;
			}
			std::string ent = out.substr(i + 1, sem - (i + 1));
			if (ent == "apos") {
				decoded.push_back('\'');
			}
			else if (ent == "quot") {
				decoded.push_back('"');
			}
			else if (ent == "amp") {
				decoded.push_back('&');
			}
			else if (ent == "lt") {
				decoded.push_back('<');
			}
			else if (ent == "gt") {
				decoded.push_back('>');
			}
			else if (!ent.empty() && ent[0] == '#') {
				// Numeric entity: decimal (e.g. &#39;) or hex (e.g. &#x27;)
				try {
					unsigned int val = 0;
					if (ent.size() > 1 && (ent[1] == 'x' || ent[1] == 'X')) {
						val = std::stoul(ent.substr(2), nullptr, 16);
					}
					else {
						val = std::stoul(ent.substr(1), nullptr, 10);
					}
					appendCodepointUtf8(decoded, val);
				}
				catch (...) {
					// If parsing fails, fall back to copying the original entity literally
					decoded.append(out, i, sem - i + 1);
				}
			}
			else {
				// Unknown entity: keep it as-is
				decoded.append(out, i, sem - i + 1);
			}
			i = sem + 1;
		}
		else {
			decoded.push_back(out[i++]);
		}
	}

	return decoded;
}
