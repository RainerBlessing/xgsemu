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
#ifndef SDLPANEL_H
#define SDLPANEL_H

class SDL_Surface ;
class Video;
class Cpu;
class FileHandler;
/**
	@author Rainer Blessing <rainer@theblessing.net>
*/
class SDLPanel : public wxPanel
{
	DECLARE_CLASS(SDLPanel)
	DECLARE_EVENT_TABLE()

private:
	SDL_Surface *screen;
	Video *video;
	Cpu *cpu;
	wxUint16* joystickr;
	FileHandler* fh;
	/**
	 * Called to paint the panel.
	 *
	 * @param event The triggering wxPaintEvent (unused).
	 */
	void onPaint(wxPaintEvent &event);

	/**
	 * Called to erase the background.
	 *
	 * @param event The triggering wxEraseEvent (unused).
	 */
	void onEraseBackground(wxEraseEvent &event);

	/**
	 * Called to update the panel.
	 *
	 * @param event The triggering wxIdleEvent (unused).
	 */
	void onIdle(wxIdleEvent &event);

	/**
	 * Creates the SDL_Surface used by this SDLPanel.
	 */
	void createScreen();
	void SDLPanel::onKeyDown(wxKeyEvent &event);
	void SDLPanel::onKeyUp(wxKeyEvent &event);
public:
	/**
	 * Creates a new SDLPanel.
	 *
	 * @param parent The wxWindow parent.
	 */
	SDLPanel(wxWindow *parent);

	/**
	 * Destructs this SDLPanel.
	 */
	~SDLPanel();
	void SDLPanel::load(const char* path);
	void SDLPanel::reset();
};

#endif
