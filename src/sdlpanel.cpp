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
#include "xgsemuframe.h"

inline void SDLPanel::onEraseBackground(wxEraseEvent &)
{}

IMPLEMENT_CLASS(SDLPanel, wxPanel)

// Timer-ID definieren
const int ID_TIMER = 1001;

BEGIN_EVENT_TABLE(SDLPanel, wxPanel)
EVT_PAINT(SDLPanel::onPaint)
EVT_ERASE_BACKGROUND(SDLPanel::onEraseBackground)
EVT_TIMER(ID_TIMER, SDLPanel::onTimer)
EVT_KEY_DOWN(SDLPanel::onKeyDown)
EVT_KEY_UP(SDLPanel::onKeyUp)
END_EVENT_TABLE()

SDLPanel::SDLPanel(XGSEmuFrame *parent) : wxPanel(parent, ID_PANEL), screen(NULL)
{
    // ensure the size of the wxPanel
    //wxSize size(560, 520);

    //SetMinSize(size);
    //SetMaxSize(size);
    cpu = 0;
    video = new Video();
    screen = video->getScreen();
    parentFrame = parent;
    SetFocus();
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    
    
    // Timer erstellen und starten (16ms ≈ 60fps)
    timer = new wxTimer(this, ID_TIMER);
    timer->Start(16);
}

SDLPanel::~SDLPanel()
{
    // Timer stoppen und löschen
    if (timer) {
        timer->Stop();
        delete timer;
    }
    
    delete video;
    delete cpu;
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
    wxSize currentSize = GetClientSize();

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

   if(parentFrame->IsFullScreen()){
        dc.SetTextForeground(wxColour(200, 200, 200, 180));
      dc.SetFont(wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                        wxFONTWEIGHT_NORMAL));
      wxString hint = wxT("F11 = Vollbild beenden");
      wxSize textSize = dc.GetTextExtent(hint);
      dc.DrawText(hint, 10, 10);
    } 
}

void SDLPanel::onTimer(wxTimerEvent &)
{
    // CPU-Ausführung mit einer festen Anzahl von Instruktionen pro Frame
    if (cpu != 0) {
        // 80 MHz (80 Millionen Instruktionen pro Sekunde) bei 60fps
        int instructionsPerFrame = 1333333;
        for (int i = 0; i < instructionsPerFrame; i++) {
            cpu->execute();
        }
    }
    
    // Bildschirm aktualisieren
    Refresh(false);
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
    if (video != 0)delete video;
    video = new Video();
    screen = video->getScreen();
    if (cpu != 0)delete cpu;
    cpu = new Cpu(video);
    joystickr = cpu->getJoystrickr();    
    FileHandler fh;
    fh.readFile (path, cpu->getROM());
}

void SDLPanel::reset()
{
    if (cpu != 0)cpu->reset();
}
