
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