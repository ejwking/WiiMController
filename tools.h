
#pragma once

#include <string>

extern int LoadEditboxFont(CWnd *pEditbox);
extern CString Utf8(const std::string& s);
extern std::string HexToUtf8(const std::string& hex);