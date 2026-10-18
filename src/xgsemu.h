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

class Video;
class Cpu;
class Bu8763;

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
    void openAudio();
    void closeAudio();
    static void audioCallback(void* userdata, Uint8* stream, int length);
    
    // Helper methods
    bool isHexOrObjFile(const std::string& path);
    void saveConfig(const std::string& key, const std::string& value);
    std::string loadConfig(const std::string& key, const std::string& defaultValue = "");
    
    // SDL2 components
    SDL_Window* window;         // SDL2 Fenster
    SDL_Renderer* renderer;     // SDL2 Renderer
    SDL_Texture* texture;       // SDL2 Textur für das Emulator-Display
    
    // Menu state (simplified)
    bool menuActive;
    
    // Emulator components
    Video* video;
    Cpu* cpu;
    Uint16* joystickr;
    Uint16 pressedUnseen = 0;   // buttons pressed on the page that no frame has run with yet
    Uint16 releaseDeferred = 0; // buttons released before a frame saw them
    Bu8763* sound;
    SDL_AudioDeviceID audioDevice;
    int audioSampleRate;
    
    // State variables
    bool running;
    bool isFullscreen;
    std::string appDir;
    std::string title;
    std::string lastDirectory;
    
    // Constants
    // 4:3 like a television, as high as the picture (525 lines)
    const int WINDOW_WIDTH = 700;
    const int WINDOW_HEIGHT = 525;
    const int TARGET_FPS = 60;
#ifdef __EMSCRIPTEN__
    double lastFrameMs = 0;
    double frameDebtMs = 0;
    double lastSpeedReportMs = 0;
    int emulatedFrames = 0;
#endif
    // The speed of the SX52 is given by the clock cycles, not by the number of
    // instructions (jmp and call take 3 cycles, iread 4).
    const unsigned int CYCLES_PER_FRAME = 1333333; // 80MHz/60fps
};

#endif // XGSEMU_H
