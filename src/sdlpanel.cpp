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
#include <wx/dcbuffer.h>
#include <wx/image.h>

#include "SDL.h"

#include "xgsemu.h"
#include "sdlpanel.h"
#include "Video.h"
#include "cpu.h"
#include "filehandler.h"

inline void SDLPanel::onEraseBackground(wxEraseEvent &)
{}

IMPLEMENT_CLASS(SDLPanel, wxPanel)

BEGIN_EVENT_TABLE(SDLPanel, wxPanel)
EVT_PAINT(SDLPanel::onPaint)
EVT_ERASE_BACKGROUND(SDLPanel::onEraseBackground)
EVT_IDLE(SDLPanel::onIdle)
EVT_KEY_DOWN(SDLPanel::onKeyDown)
EVT_KEY_UP(SDLPanel::onKeyUp)
END_EVENT_TABLE()

SDLPanel::SDLPanel(wxWindow *parent) : wxPanel(parent, ID_PANEL), screen(NULL)
{

	// ensure the size of the wxPanel
	wxSize size(560, 520);

	SetMinSize(size);
	SetMaxSize(size);
	cpu = 0;
	video = new Video();
	screen = video->getScreen();
	//fh.readFile ("/home/rainer/src/sx/ntsc/Racer/racer_01.hex", ROM);
	//fh.readFile ("/home/rainer/src/sx/ntsc/Pac_Man/rem_pac_01.hex", ROM);
	SetFocus();
	//fh.readFile ("/home/rainer/src/sx/ntsc/Plasma/mic_plasma_01.hex", ROM);

}

SDLPanel::~SDLPanel()
{
	if (screen != NULL)
	{
		SDL_FreeSurface(screen);
	}
	delete video;
	delete cpu;
	delete fh;
}

void SDLPanel::onPaint(wxPaintEvent &)
{
	// can't draw if the screen doesn't exist yet
	if (screen == NULL)
	{
		return ;
	}

	// lock the surface if necessary
	if (SDL_MUSTLOCK(screen))
	{
		if (SDL_LockSurface(screen) < 0)
		{
			return ;
		}
	}

	// create a bitmap from our pixel data
	wxBitmap bmp(wxImage(screen->w, screen->h,
	                     static_cast<unsigned char *>(screen->pixels), true));

	// unlock the screen
	if (SDL_MUSTLOCK(screen))
	{
		SDL_UnlockSurface(screen);
	}

	// paint the screen
	wxBufferedPaintDC dc(this, bmp);
}

void SDLPanel::onIdle(wxIdleEvent &)
{
	// refresh the panel
	Refresh(false);
	if (cpu != 0)cpu->execute();
}

void SDLPanel::onKeyDown(wxKeyEvent &event)
{
	switch (event.m_keyCode)
	{
			case WXK_CONTROL:
			*joystickr |= 0x10;
			break;
			case WXK_UP:
			*joystickr |= 0x1;
			break;
			case WXK_RIGHT:
			*joystickr |= 0x8;
			break;
			case WXK_DOWN:
			*joystickr |= 0x2;
			break;
			case WXK_LEFT:
			*joystickr |= 0x4;
			break;
			default:
			// allow other handlers to process KEY_DOWN events
			event.Skip();
			break;
	}

}

void SDLPanel::onKeyUp(wxKeyEvent &event)
{
	switch (event.m_keyCode)
	{
			case WXK_CONTROL:
			*joystickr &= ~0x10;
			break;
			case WXK_UP:
			*joystickr &= ~ 0x1;
			break;
			case WXK_RIGHT:
			*joystickr &= ~0x8;
			break;
			case WXK_DOWN:
			*joystickr &= ~0x2;
			break;
			case WXK_LEFT:
			*joystickr &= ~0x4;
			break;
			default:
			// allow other handlers to process KEY_DOWN events
			event.Skip();
			break;
	}

}

void SDLPanel::load(const char* path)
{
	if (video == 0)delete video;
	video = new Video();
	screen = video->getScreen();
	if (cpu == 0)delete cpu;
	cpu = new Cpu(video);
	joystickr = cpu->getJoystrickr();
	USHORT* ROM = cpu->getROM();
	FileHandler fh;
	fh.readFile (path, ROM);
}

void SDLPanel::reset()
{
	if (cpu != 0)cpu->reset();
}
