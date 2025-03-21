/***************************************************************************
*   Copyright (C) 2005 by Rainer Blessing   *
*   rainer@theblessing.net   *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
***************************************************************************/
#include <wx/wx.h>
#include <wx/config.h>
#include "wx/wxhtml.h"
#include "wx/statline.h"

#include "sdlpanel.h"
#include "xgsemu.h"
#include "xgsemuframe.h"


BEGIN_EVENT_TABLE( XGSEmuFrame, wxFrame )
EVT_MENU( Menu_File_Quit, XGSEmuFrame::OnQuit )
EVT_MENU( Menu_File_Load, XGSEmuFrame::OnLoad )
EVT_MENU( Menu_File_Reset, XGSEmuFrame::OnReset )
EVT_MENU( Menu_View_Fullscreen, XGSEmuFrame::OnFullScreen )
EVT_MENU( Menu_Help_About, XGSEmuFrame::OnAbout )
EVT_MENU( Menu_Help_ReadMe, XGSEmuFrame::OnReadMe )
EVT_KEY_DOWN(XGSEmuFrame::OnKeyDown)
END_EVENT_TABLE()


XGSEmuFrame::XGSEmuFrame(const wxString& appDir, const wxString& title, const wxPoint& pos, const wxSize& size, long style )
: wxFrame((wxFrame *)NULL, -1, title, pos, size, style),isFullScreen(false)
{
	this->appDir = appDir;
	wxMenu *menuFile = new wxMenu;

	menuFile->Append( Menu_File_Load, wxT( "L&oad" ) );
	menuFile->Append( Menu_File_Reset, wxT( "&Reset" ) );
	menuFile->AppendSeparator();
	menuFile->Append( Menu_File_Quit, wxT( "E&xit" ) );
	
	wxMenu *menuView = new wxMenu;
	menuView->Append( Menu_View_Fullscreen, wxT( "Fullscreen\tF11" ) );

	wxMenu *menuHelp = new wxMenu;
	menuHelp->Append( Menu_Help_ReadMe, wxT( "View &Readme..." ) );
	menuHelp->Append( Menu_Help_About, wxT( "&About..." ) );

	wxMenuBar *menuBar = new wxMenuBar;
	menuBar->Append( menuFile, wxT( "&File" ) );
	menuBar->Append( menuView, wxT( "&View" ) );
	menuBar->Append( menuHelp, wxT( "&Help" ) );

	SetMenuBar( menuBar );

	//fCreateStatusBar();
	panel = new SDLPanel(this);

	wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
	sizer->Add(panel, 1, wxEXPAND | wxALL, 10);
	SetSizer(sizer);


}

void
XGSEmuFrame::OnQuit( wxCommandEvent& WXUNUSED( event ) )
{
	Close(TRUE);
}

void
XGSEmuFrame::OnAbout( wxCommandEvent& WXUNUSED( event ) )
{
	wxMessageBox( wxT( "(c) 2005 Rainer Blessing" ),
	              wxT( "About XGSEmu" ), wxOK | wxICON_INFORMATION, this );
}

void
XGSEmuFrame::OnLoad( wxCommandEvent& WXUNUSED( event ) )
{
	wxString directory = wxT("");
	wxConfig *config = new wxConfig(wxT("XGSEmu"));
	config->Read(wxT("LoadDirectory"), &directory);
	wxFileDialog d( this, _("Choose a file"), directory, wxT(""), wxT("*.hex;*.obj"), 
                wxFD_OPEN | wxFD_FILE_MUST_EXIST, 
                wxDefaultPosition, 
                wxDefaultSize, 
                wxT("XGS File Selector"));
	if (d.ShowModal() == wxID_OK)
	{
		panel->load(d.GetPath().mb_str());
		directory = d.GetDirectory();
		config->Write(wxT("LoadDirectory"), directory);
	}
	delete config;
}

void
XGSEmuFrame::OnReset( wxCommandEvent& WXUNUSED( event ) )
{
	panel->reset();
}

void XGSEmuFrame::OnReadMe(wxCommandEvent& WXUNUSED(event))
{
	wxBoxSizer *topsizer;
	wxHtmlWindow *html;
	wxDialog dlg(this, wxID_ANY, wxString(_("About")));

	topsizer = new wxBoxSizer(wxVERTICAL);



	html = new wxHtmlWindow(&dlg, wxID_ANY, wxDefaultPosition, wxSize(300, 160), wxHW_SCROLLBAR_NEVER);
	html -> SetBorders(0);
	html -> LoadPage(appDir + wxT("/README.htm"));
	html -> SetSize(html -> GetInternalRepresentation() -> GetWidth(),
	                html -> GetInternalRepresentation() -> GetHeight());

	topsizer -> Add(html, 1, wxALL, 10);

	wxButton *bu1 = new wxButton(&dlg, wxID_OK, _("OK"));
	bu1 -> SetDefault();

	topsizer -> Add(bu1, 0, wxALL | wxALIGN_RIGHT, 15);

	dlg.SetSizer(topsizer);
	topsizer -> Fit(&dlg);

	dlg.ShowModal();
}

void XGSEmuFrame::LoadHexFile(const wxString& path)
{
    // Überprüfen, ob die Datei existiert
    if (!wxFileExists(path)) {
        wxMessageBox(wxString::Format(_("Error: File '%s' does not exist."), path), 
                    _("File Error"), wxOK | wxICON_ERROR);
        return;
    }
    
    // Überprüfen, ob es sich um eine .hex oder .obj Datei handelt
    wxFileName fileName(path);
    wxString extension = fileName.GetExt().Lower();
    
    if (extension != wxT("hex") && extension != wxT("obj")) {
        wxMessageBox(wxString::Format(_("Error: File '%s' is not a .hex or .obj file."), path), 
                    _("Invalid File Type"), wxOK | wxICON_ERROR);
        return;
    }
    
    // Laden der Datei
    panel->load(path.mb_str());
    
    // Optional: Aktuelles Verzeichnis in der Konfiguration speichern
    wxConfig *config = new wxConfig(wxT("XGSEmu"));
    config->Write(wxT("LoadDirectory"), fileName.GetPath());
    delete config;
}

  void XGSEmuFrame::ToggleFullScreen() {
    isFullScreen = !isFullScreen;
    ShowFullScreen(isFullScreen, wxFULLSCREEN_ALL);
  }

 void XGSEmuFrame::OnKeyDown(wxKeyEvent &event) {
    // F11-Taste für Vollbild
    if (event.GetKeyCode() == WXK_F11) {
      ToggleFullScreen();
      return;
    }
    // ESC-Taste zum Beenden des Vollbildmodus
    else if (event.GetKeyCode() == WXK_ESCAPE && isFullScreen) {
      ShowFullScreen(false);
      isFullScreen = false;
      return;
    }

    event.Skip();
  }
