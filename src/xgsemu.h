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
#include <map>

class Video;
class Cpu;

const int WINDOW_WIDTH = 560;  // Entspricht der Surface-Breite in Video.cpp
const int WINDOW_HEIGHT = 525; // Entspricht der Surface-Höhe in Video.cpp

/**
 * SDL2-based XGS Emulator
 */
class XGSEmu {
public:
    XGSEmu(const std::string& appDir, const std::string& title);
    ~XGSEmu();
    
    int run(const std::string& hexFilePath = "");
    void loadHexFile(const std::string& path);
    void reset();
    void toggleFullscreen();
    
private:
    // Main loop and event handling
    void handleEvents();
    void update();
    void render();
    void updateTexture();
    void openFileDialog();
    
    // Helper methods
    bool isHexOrObjFile(const std::string& path);
    void saveConfig(const std::string& key, const std::string& value);
    std::string loadConfig(const std::string& key, const std::string& defaultValue = "");
    
    // SDL2 components
    SDL_Window* window;         // SDL2 Fenster
    SDL_Renderer* renderer;     // SDL2 Renderer
    SDL_Texture* texture;       // SDL2 Textur für das Emulator-Display
    SDL_Surface* emuDisplay;    // Surface für das Emulator-Display (vom Video-System)
    
    // Menu state (simplified)
    bool menuActive;
    
    // Emulator components
    Video* video;
    Cpu* cpu;
    Uint16* joystickr;
    
    // State variables
    bool running;
    bool isFullscreen;
    std::string appDir;
    std::string title;
    std::string lastDirectory;
    
    // Constants
    const int WINDOW_WIDTH = 561;
    const int WINDOW_HEIGHT = 550;
    const int TARGET_FPS = 60;
    const int FRAME_DELAY = 1000 / TARGET_FPS;
    const int INSTRUCTIONS_PER_FRAME = 1333333; // 80MHz/60fps
};

#endif // XGSEMU_H
