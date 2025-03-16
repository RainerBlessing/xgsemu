#include <wx/sysopt.h>
#include <wx/wx.h>
#include "xgsemu.h"
#include "xgsemuframe.h"

extern wxString wxFindAppPath(const wxString& argv0, const wxString& cwd, const wxString& appVariableName, const wxString& appName);


IMPLEMENT_APP(XGSEmuapp)


//bool
//XGSEmuapp::OnInit()
//{
//	wxSystemOptions::SetOption("gtk.tlw.can-set-border", "0");
//	wxString currentDir = wxGetCwd();
//	wxString appDir = wxFindAppPath(argv[0], currentDir, wxT("MYAPPDIR"), wxT("MyApp"));
//
//	wxString msg;
//	msg.Printf( _("XGSEmu %s") , wxT(XGSE_VERSION));
//	XGSEmuFrame *frame = new XGSEmuFrame(appDir, msg, wxPoint(50, 50), wxSize(561, 550), wxDEFAULT_FRAME_STYLE & ~ (wxRESIZE_BORDER | wxRESIZE_BORDER | wxMAXIMIZE_BOX) );
//
//	frame->Show(TRUE);
//	SetTopWindow(frame);
//	return TRUE;
//}


bool XGSEmuapp::OnInit()
{
    wxSystemOptions::SetOption("gtk.tlw.can-set-border", "0");
    wxString currentDir = wxGetCwd();
    wxString appDir = wxFindAppPath(argv[0], currentDir, wxT("MYAPPDIR"), wxT("MyApp"));

    // Kommandozeilenargumente verarbeiten
    m_hexFilePath = wxEmptyString;
    
    for (int i = 1; i < argc; i++) {
        // Wenn ein Argument nicht mit "-" beginnt, behandeln wir es als Pfad zur Hex-Datei
        if (argv[i][0] != '-') {
            m_hexFilePath = argv[i];
            break;
        }
    }

    wxString msg;
    msg.Printf(_("XGSEmu %s"), wxT(XGSE_VERSION));
    XGSEmuFrame *frame = new XGSEmuFrame(appDir, msg, wxPoint(50, 50), wxSize(561, 550), 
                         wxDEFAULT_FRAME_STYLE & ~(wxRESIZE_BORDER | wxRESIZE_BORDER | wxMAXIMIZE_BOX));

    frame->Show(TRUE);
    SetTopWindow(frame);
    
    // Wenn eine Hex-Datei angegeben wurde, laden und ausführen
    if (!m_hexFilePath.IsEmpty()) {
        frame->LoadHexFile(m_hexFilePath);
    }
    
    return TRUE;
}
