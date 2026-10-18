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

#ifndef XGSEMU_H
#define XGSEMU_H

#include <SDL.h>
#include <string>
#include "AudioOutput.h"
#include "Config.h"
#include "Machine.h"

/**
 * The window of the emulator: SDL2 shows the picture of the Machine, passes the keys to it
 * and plays its sound.
 */
class XGSEmu {
public:
    XGSEmu(const std::string& title);
    ~XGSEmu();
    
    int run(const std::string& hexFilePath = "");
    void loadHexFile(const std::string& path);
    void reset();
    void toggleFullscreen();
    void setMuted(bool muted);
    void setJoystick(Uint16 mask, bool down);
#ifdef __EMSCRIPTEN__
    void frame();
#endif
    
private:
    // Main loop and event handling
    void handleEvents();
    void update();
    void render();
    void updateTexture();
    void openFileDialog();
    
    // Helper methods
    bool isHexOrObjFile(const std::string& path);
    
    // SDL2 components
    SDL_Window* window;         // SDL2 Fenster
    SDL_Renderer* renderer;     // SDL2 Renderer
    SDL_Texture* texture;       // SDL2 Textur für das Emulator-Display
    
    // The emulated machine, and what is attached to it. The audio output uses the sound chip of
    // the machine, so it comes after it.
    Machine machine;
    AudioOutput audio;
    Config config;
    
    // State variables
    bool running;
    bool isFullscreen;
    std::string title;
    std::string lastDirectory;
    
    // Constants
    // 4:3 like a television, as high as the picture (525 lines)
    static const int WINDOW_WIDTH = 700;
    static const int WINDOW_HEIGHT = 525;
    static const int TARGET_FPS = 60;
#ifdef __EMSCRIPTEN__
    double lastFrameMs = 0;
    double frameDebtMs = 0;
    double lastSpeedReportMs = 0;
    int emulatedFrames = 0;
#endif
};

#endif // XGSEMU_H
