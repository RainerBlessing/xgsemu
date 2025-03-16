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
#ifndef XGSEMUFRAME_H
#define XGSEMUFRAME_H

class SDLPanel;

/**
	A wxFrame with the menubar
	@author Rainer Blessing <rainer@theblessing.net>
*/
class
XGSEmuFrame : public wxFrame
{
public:
	XGSEmuFrame( const wxString& appDir, const wxString& title, const wxPoint& pos, const wxSize& size, long style );
	void OnQuit( wxCommandEvent& event );
	void OnLoad( wxCommandEvent& event );
	void OnReset( wxCommandEvent& event );
	void OnAbout( wxCommandEvent& event );
	void OnReadMe( wxCommandEvent& event );
	void LoadHexFile(const wxString& path);

private:
	DECLARE_EVENT_TABLE()
	SDLPanel *panel;
	wxString appDir;
};

enum
{
    Menu_File_Quit = 100,
    Menu_File_Load,
    Menu_File_Reset,
    Menu_Help_About,
    Menu_Help_ReadMe,
};

#endif
