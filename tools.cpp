
#include "pch.h"
#include "tools.h"


/* 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
CHARACTER ENCODING NOTES
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

** UTF-8 **

UTF-8 is a character encoding system that translates the letters, numbers, and symbols you type into the raw binary data computers understand. 
It is the universal standard for the internet, powering the vast majority of all websites worldwide.

Here is exactly how it works in four simple rules:

1. It is a "Variable-Width" system
While older encodings used a strict, fixed number of bits per character, UTF-8 uses between 1 and 4 bytes depending on the complexity of the symbol.

1 Byte: Used for standard English letters, numbers, and basic punctuation.
2 Bytes: Used for characters with diacritics (e.g., é, ñ) and alphabets like Greek, Arabic, and Cyrillic.
3 Bytes: Used for complex characters like Chinese, Japanese, and Korean scripts, as well as many standard symbols.
4 Bytes: Used for rare symbols, mathematical notations, and all emojis

2. It is fully backward-compatible with ASCII 
If a text file only contains standard English characters (the first 128 characters of the old ASCII standard), UTF-8 encodes them in exactly 1 byte. 
The first bit of these bytes is always 0. Because old ASCII programs also use the first bit as 0, they can read simple UTF-8 files without any problems.

3. It marks "Continuation" bytes
When a character requires more than 1 byte, how does the computer know where a character begins and ends? UTF-8 solves this with a clever marker system:
Leading Byte: For characters that take 2, 3, or 4 bytes, the first byte starts with a set of 1s followed by a 0. 
The number of 1s tells the computer how many bytes make up that specific character. 
For example, a two-byte character’s leading byte will start with 110, and a three-byte character will start with 1110.
Continuation Bytes: All subsequent bytes for that character will always begin with 10.

4. It acts as an algorithmic map
Every character in the world gets a unique "code point" (a hexadecimal reference number, like U+0041 for the letter "A" or U+1F600 for the grinning face emoji). 
UTF-8 doesn't just store these characters as raw, massive numbers; it applies a specific mathematical algorithm to pack those numbers into the 1-to-4-byte byte sequences described above.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

modern C++ advice. Internally: Use UTF-8 everywhere if possible.

Meaning:
	std::string  (std::string does NOT mean ASCII/ANSI, it just means sequence of bytes).
	UTF-8 encoded text
	JSON UTF-8
	network UTF-8
This massively simplifies things. Then only convert at Windows API boundaries

Windows internally still prefers UTF-16:
	wchar_t
	std::wstring
	CreateWindowW(...)
	SetWindowTextW(...)
	etc

So the clean architecture is:

Logic/Network/JSON/files:
	UTF-8 std::string
Windows UI boundary:
	convert UTF-8 to UTF-16

Recommended architecture
	1. Receive HTTP response as raw bytes, (libcurl already does this)
	2. Treat JSON as UTF-8, (nlohmann/json already assumes UTF-8)
	3. Decode hex metadata to bytes
	4. Treat decoded metadata as UTF-8 text
	5. Convert UTF-8 to UTF-16 only for Win32 UI

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
UTF-8 files

You generally want: std::ifstream  not - std::wifstream
Read UTF-8 text files as raw bytes, file contents are UTF-8 bytes stored in std::string

Why std::wifstream is usually NOT ideal now
	uses wchar_t
	locale-dependent behavior
	platform differences
	messy codecvt conversions
	legacy Unicode model
Even Microsoft is moving that direction now.

// Tiny robust UTF-8 file loader. 
//
// std::ifstream file(path, std::ios::binary);
// std::string text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
//
// Reads the entire file into a std::string, which can contain UTF-8 data without any issues, as std::string is just a byte container and does not interpret the data as characters. This 
// avoids all the complications of using wide-character file streams and character encoding conversions when dealing with Unicode file paths and content on Windows. The file is read in binary 
// mode to ensure that no newline translations or other transformations are applied, preserving the exact bytes of the file content, which is important for correctly handling UTF-8 encoded text.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Source code files must also be encoded as UTF-8 without BOM, to avoid any issues with character encoding in the source code itself. This ensures that any string 
literals in the code are correctly interpreted as UTF-8, and that the compiler can handle them without any encoding-related problems. Using UTF-8 for source files 
is a best practice in modern C++ development, especially when dealing with internationalization and Unicode text.
(I checked - VS does save source files as UTF-8 without BOM by default, "Unicode (UTF-8 without signature) - Code page 65001." )

example - If you ever type non-ASCII characters directly into source:

std::string artist = u8"Björk";
UTF-8 source encoding becomes very important.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

CString Utf8(const std::string& s)
{
	return CString(CA2W(s.c_str(), CP_UTF8));
}

/*
// Convert UTF-8 (std::string) to UTF-16 (std::wstring)
std::wstring Utf8ToUtf16(const std::string& utf8Str)
{
	if (utf8Str.empty()) return L"";

	int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, &utf8Str[0], (int)utf8Str.size(), NULL, 0);
	std::wstring utf16Str(sizeNeeded, 0);
	MultiByteToWideChar(CP_UTF8, 0, &utf8Str[0], (int)utf8Str.size(), &utf16Str[0], sizeNeeded);

	return utf16Str;
}

// Convert UTF-16 (std::wstring) to UTF-8 (std::string)
std::string Utf16ToUtf8(const std::wstring& utf16Str)
{
	if (utf16Str.empty()) return "";

	int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, &utf16Str[0], (int)utf16Str.size(), NULL, 0, NULL, NULL);
	std::string utf8Str(sizeNeeded, 0);
	WideCharToMultiByte(CP_UTF8, 0, &utf16Str[0], (int)utf16Str.size(), &utf8Str[0], sizeNeeded, NULL, NULL);

	return utf8Str;
}
*/





/*
std::string CStringToUtf8(const CString& s)
{
	if (s.IsEmpty())
		return std::string();
#ifdef UNICODE
	int utf8len = ::WideCharToMultiByte(CP_UTF8, 0, s, -1, nullptr, 0, nullptr, nullptr);
	if (utf8len <= 0)
		return std::string();
	std::string out(static_cast<size_t>(utf8len), '\0');
	::WideCharToMultiByte(CP_UTF8, 0, s, -1, &out[0], utf8len, nullptr, nullptr);
	if (!out.empty() && out.back() == '\0')
		out.pop_back();
	return out;
#else
	// ANSI build: convert from current ANSI codepage to UTF-8
	int wlen = ::MultiByteToWideChar(CP_ACP, 0, CT2A(s), -1, nullptr, 0);
	if (wlen <= 0)
		return std::string();
	std::wstring w(static_cast<size_t>(wlen), L'\0');
	::MultiByteToWideChar(CP_ACP, 0, CT2A(s), -1, &w[0], wlen);
	int utf8len = ::WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, nullptr, 0, nullptr, nullptr);
	if (utf8len <= 0)
		return std::string();
	std::string out(static_cast<size_t>(utf8len), '\0');
	::WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, &out[0], utf8len, nullptr, nullptr);
	if (!out.empty() && out.back() == '\0')
		out.pop_back();
	return out;
#endif
}
*/

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
