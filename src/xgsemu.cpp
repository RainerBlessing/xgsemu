#include <wx/wx.h>
#include "xgsemu.h"
#include "xgsemuframe.h"

extern wxString wxFindAppPath(const wxString& argv0, const wxString& cwd, const wxString& appVariableName, const wxString& appName);

IMPLEMENT_APP(XGSEmuapp)


bool
XGSEmuapp::OnInit()
{
	wxString currentDir = wxGetCwd();
	wxString appDir = wxFindAppPath(argv[0], currentDir, wxT("MYAPPDIR"), wxT("MyApp"));

	wxString msg;
	msg.Printf( _("XGSEmu %s") , wxT(XGSE_VERSION));
	XGSEmuFrame *frame = new XGSEmuFrame(appDir, msg, wxPoint(50, 50), wxSize(527, 550), wxDEFAULT_FRAME_STYLE & ~ (wxRESIZE_BORDER | wxRESIZE_BOX | wxMAXIMIZE_BOX) );

	frame->Show(TRUE);
	SetTopWindow(frame);
	return TRUE;
}



