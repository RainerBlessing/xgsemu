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

#include "xgsemu.h"
#include "Video.h"
#include "DisplayLayout.h"
#include "InputMap.h"
#include <fstream>
#include <algorithm>
#include <dirent.h>
#include <vector>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

// The browser calls this (see web/shell.html) to start a program that was put
// into the virtual file system.
static XGSEmu* webEmu = NULL;
extern "C" EMSCRIPTEN_KEEPALIVE void xgsLoad(const char* path) {
    if (webEmu != NULL) {
        webEmu->loadHexFile(path);
    }
}
extern "C" EMSCRIPTEN_KEEPALIVE const char* xgsVersion() {
    return XGSE_VERSION;
}
extern "C" EMSCRIPTEN_KEEPALIVE void xgsMute(int muted) {
    if (webEmu != NULL) {
        webEmu->setMuted(muted != 0);
    }
}
// The on-screen controls of the page press and release joystick bits
// (1 up, 2 down, 4 left, 8 right, 0x10 fire), like the keys do.
extern "C" EMSCRIPTEN_KEEPALIVE void xgsJoystick(int mask, int down) {
    if (webEmu != NULL) {
        webEmu->setJoystick(mask, down != 0);
    }
}
extern "C" EMSCRIPTEN_KEEPALIVE void xgsReset() {
    if (webEmu != NULL) {
        webEmu->reset();
    }
}
#endif

XGSEmu::XGSEmu(const std::string& title)
    : window(NULL),
      renderer(NULL),
      texture(NULL),
      audio(machine.getSound()),
      config(Config::defaultPath()),
      running(true),
      isFullscreen(false),
      title(title)
{
    // Initialize SDL2
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL konnte nicht initialisiert werden! SDL-Fehler: %s\n", SDL_GetError());
        running = false;
        return;
    }
    
    // Erstelle Fenster mit SDL2
    window = SDL_CreateWindow(
        title.c_str(),
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    
    if (window == NULL) {
        printf("Fenster konnte nicht erstellt werden! SDL-Fehler: %s\n", SDL_GetError());
        running = false;
        return;
    }
    
    // Erstelle Renderer
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == NULL) {
        printf("Renderer konnte nicht erstellt werden! SDL-Fehler: %s\n", SDL_GetError());
        running = false;
        return;
    }
    
    // Setze Renderer-Farbe auf Schwarz
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    
    // Erstelle Textur für das Emulator-Display
    updateTexture();
    
    // Lade letzte Verzeichniskonfiguration
    lastDirectory = config.get("LoadDirectory", ".");

    audio.open();
}

void XGSEmu::setJoystick(Uint16 mask, bool down) {
    machine.setJoystick(mask, down);
}

void XGSEmu::setMuted(bool muted) {
    audio.setMuted(muted);
}

XGSEmu::~XGSEmu() {
    // Der Audio-Thread greift auf den Soundchip zu, deshalb zuerst schliessen.
    audio.close();

    // Clean up SDL2 components
    if (texture != NULL) {
        SDL_DestroyTexture(texture);
    }
    
    if (renderer != NULL) {
        SDL_DestroyRenderer(renderer);
    }
    
    if (window != NULL) {
        SDL_DestroyWindow(window);
    }
    
    // Quit SDL2
    SDL_Quit();
}

int XGSEmu::run(const std::string& hexFilePath) {
    printf("XGSEmu::run gestartet mit Datei: %s\n", hexFilePath.empty() ? "(keine)" : hexFilePath.c_str());
    
    if (!running) {
        printf("XGSEmu wurde nicht korrekt initialisiert.\n");
        return 1; // Error during initialization
    }
    
    // Load hex file if provided
    if (!hexFilePath.empty()) {
        loadHexFile(hexFilePath);
    }
    
#ifdef __EMSCRIPTEN__
    // The browser owns the main loop: it calls frame() for every animation frame.
    webEmu = this;
    lastFrameMs = emscripten_get_now();
    EM_ASM({ if (Module.onEmulatorReady) Module.onEmulatorReady(); });
    emscripten_set_main_loop_arg([](void* self) { static_cast<XGSEmu*>(self)->frame(); }, this, 0, 1);
    return 0;
#endif

    // Hauptschleife
    printf("Starte Hauptschleife...\n");
    // The frame time is not a whole number of milliseconds (16.67 ms), so the
    // time of the next frame is accumulated instead of waiting a fixed delay.
    const Uint64 counterFrequency = SDL_GetPerformanceFrequency();
    const Uint64 frameTicks = counterFrequency / TARGET_FPS;
    Uint64 nextFrame = SDL_GetPerformanceCounter();
    
    try {
        while (running) {
            // Führe einen Frame aus
            handleEvents();
            
            if (machine.isLoaded()) {
                update();
            } else {
                // Kein Programm - Warte auf Benutzeraktion
                SDL_Delay(100);
            }
            
            render();
            
            // Frame-Begrenzung
            nextFrame += frameTicks;
            Uint64 now = SDL_GetPerformanceCounter();
            if (nextFrame > now) {
                SDL_Delay((Uint32)((nextFrame - now) * 1000 / counterFrequency));
            } else if (now - nextFrame > frameTicks * 5) {
                // Far behind (menu, window drag): do not try to catch up
                nextFrame = now;
            }
            
        }
    } catch (std::exception& e) {
        printf("Exception in Hauptschleife: %s\n", e.what());
        return 1;
    } catch (...) {
        printf("Unbekannte Exception in Hauptschleife!\n");
        return 1;
    }
    
    printf("Hauptschleife beendet.\n");
    return 0;
}

#ifdef __EMSCRIPTEN__
void XGSEmu::frame() {
    // Animation frames come with the refresh rate of the display (60, 120, 144 Hz),
    // the emulated machine runs at 60 frames per second whatever that is.
    const double frameMs = 1000.0 / TARGET_FPS;
    double now = emscripten_get_now();
    frameDebtMs += now - lastFrameMs;
    lastFrameMs = now;
    if (frameDebtMs > frameMs * 5) {
        frameDebtMs = frameMs * 5; // tab was in the background: do not catch up
    }

    handleEvents();
    bool ran = false;
    while (frameDebtMs >= frameMs) {
        frameDebtMs -= frameMs;
        if (machine.isLoaded()) {
            machine.runFrame();
            ran = true;
            emulatedFrames++;
        }
    }
    if (now - lastSpeedReportMs >= 1000) {
        if (machine.isLoaded()) {
            // emulated frames per second: 60 means full speed
            double fps = emulatedFrames * 1000.0 / (now - lastSpeedReportMs);
            EM_ASM({ if (Module.onSpeed) Module.onSpeed($0); }, fps);
        }
        emulatedFrames = 0;
        lastSpeedReportMs = now;
    }
    if (ran) {
        updateTexture();
    }
    render();
}
#endif

void XGSEmu::handleEvents() {
    SDL_Event event;
    
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                running = false;
                break;
                
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE:
#ifndef __EMSCRIPTEN__
                        running = false;
#endif
                        break;
                        
                    case SDLK_F11:
                        toggleFullscreen();
                        break;
                        
                    case SDLK_F5:
                        reset();
                        break;
                        
                    case SDLK_l:
                        if ((event.key.keysym.mod & KMOD_CTRL) != 0) {
                            // Ctrl+L for load file
                            openFileDialog();
                        }
                        break;
                        
                    default:
                        if (unsigned short button = joystickMaskForKey(event.key.keysym.sym)) {
                            machine.setJoystick(button, true);
                        }
                        break;
                }
                break;
                
            case SDL_KEYUP:
                if (unsigned short button = joystickMaskForKey(event.key.keysym.sym)) {
                    machine.setJoystick(button, false);
                }
                break;
                
            case SDL_WINDOWEVENT:
                // Handle window events
                if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                    // Window was resized
                    // In SDL2, we don't need to recreate the renderer or window
                    // SDL2 handles this automatically
                }
                break;
        }
    }
}

void XGSEmu::update() {
    machine.runFrame();
    updateTexture();
}

void XGSEmu::updateTexture() {
    if (renderer == NULL) {
        return;
    }
    const Video& video = machine.getVideo();
    if (texture == NULL) {
        // the pixels of Video are 0xAARRGGBB
        texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
                                    video.width(), video.height());
        if (texture == NULL) {
            printf("Textur konnte nicht erstellt werden! SDL-Fehler: %s\n", SDL_GetError());
            return;
        }
    }
    SDL_UpdateTexture(texture, NULL, video.pixels(), video.pitch());
}

void XGSEmu::render() {
    if (renderer && texture) {
        // Clear renderer
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        
        // Get current window size
        int windowWidth, windowHeight;
        SDL_GetWindowSize(window, &windowWidth, &windowHeight);
        
        // The picture is shown in 4:3 like on a television, whatever the shape of the
        // emulated picture is (560 x 525). It is stretched and centered.
        DisplayRect target = fitAspect(windowWidth, windowHeight, 4, 3);
        SDL_Rect destRect;
        destRect.x = target.x;
        destRect.y = target.y;
        destRect.w = target.w;
        destRect.h = target.h;
        
        // Render the texture
        SDL_RenderCopy(renderer, texture, NULL, &destRect);
        
        // Update the screen
        SDL_RenderPresent(renderer);
    }
}

void XGSEmu::loadHexFile(const std::string& path) {
    // Ausführliche Ausgabe für Debug-Zwecke
    printf("Versuche Datei zu laden: %s\n", path.c_str());
    
    // Check if file exists and has valid extension
    if (!isHexOrObjFile(path)) {
        printf("Error: File '%s' does not exist or is not a .hex or .obj file.\n", path.c_str());
        return;
    }
    
    printf("Lade ROM-Datei: %s\n", path.c_str());
    // If the file cannot be read the running program stays as it is
    if (!machine.load(path)) {
        printf("Fehler beim Lesen der ROM-Datei!\n");
        return;
    }
    printf("ROM-Datei erfolgreich geladen.\n");
    updateTexture();
    
    // Save directory in config
    size_t lastSlash = path.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        lastDirectory = path.substr(0, lastSlash);
        config.set("LoadDirectory", lastDirectory);
    }
}

void XGSEmu::reset() {
    machine.reset();
}

void XGSEmu::toggleFullscreen() {
    isFullscreen = !isFullscreen;
    
    if (isFullscreen) {
        // SDL2 makes fullscreen much easier
        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
    } else {
        // Back to windowed mode
        SDL_SetWindowFullscreen(window, 0);
    }
}

bool XGSEmu::isHexOrObjFile(const std::string& path) {
    // Check if file exists
    std::ifstream file(path);
    if (!file.good()) {
        return false;
    }
    
    // Check extension
    size_t dotPos = path.find_last_of('.');
    if (dotPos == std::string::npos) {
        return false;
    }
    
    std::string ext = path.substr(dotPos + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    return (ext == "hex" || ext == "obj");
}

void XGSEmu::openFileDialog() {
    // In einer realen Implementierung würde dies ein richtiger Dateidialog sein
    // Für jetzt verwenden wir eine einfache Simulation eines Dateidialogs

    // Rufe Liste von .hex und .obj Dateien im aktuellen Verzeichnis ab
    std::vector<std::string> files;
    
    DIR* dp = opendir(lastDirectory.c_str());
    if (dp != NULL) {
        struct dirent* entry;
        while ((entry = readdir(dp)) != NULL) {
            std::string filename = entry->d_name;
            if (filename.size() > 4) {
                std::string ext = filename.substr(filename.size() - 4);
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                if (ext == ".hex" || ext == ".obj") {
                    files.push_back(filename);
                }
            }
        }
        closedir(dp);
    }
    
    // The files come in the order of the directory, which is arbitrary: take the first by name.
    std::sort(files.begin(), files.end());

    // Wenn es Dateien gibt, lade die erste (in einer realen Implementierung würden wir den Benutzer wählen lassen)
    if (!files.empty()) {
        std::string fullPath = lastDirectory + "/" + files[0];
        loadHexFile(fullPath);
        printf("Datei geladen: %s\n", fullPath.c_str());
    } else {
        printf("Keine .hex oder .obj Dateien im Verzeichnis gefunden: %s\n", lastDirectory.c_str());
    }
}
