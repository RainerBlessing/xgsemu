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
#include "cpu.h"
#include "filehandler.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <sys/stat.h>
#include <dirent.h>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#endif

// Helper function to get configuration directory
std::string getConfigDir() {
    std::string configDir;
    
#ifdef _WIN32
    char appData[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, appData))) {
        configDir = std::string(appData) + "\\XGSEmu";
        // Create directory if it doesn't exist
        CreateDirectoryA(configDir.c_str(), NULL);
    } else {
        configDir = ".";
    }
#else
    const char* homeDir = getenv("HOME");
    if (homeDir != NULL) {
        configDir = std::string(homeDir) + "/.config/xgsemu";
        // Create directory if it doesn't exist
        mkdir(configDir.c_str(), 0755);
    } else {
        configDir = ".";
    }
#endif

    return configDir;
}

XGSEmu::XGSEmu(const std::string& appDir, const std::string& title)
    : window(NULL),
      renderer(NULL),
      texture(NULL),
      menuActive(false),
      video(NULL),
      cpu(NULL),
      joystickr(NULL),
      running(true),
      isFullscreen(false),
      appDir(appDir),
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
    
    // Initialize video system
    video = new Video();
    emuDisplay = video->getScreen();
    
    // Erstelle Textur für das Emulator-Display
    updateTexture();
    
    // Lade letzte Verzeichniskonfiguration
    lastDirectory = loadConfig("LoadDirectory", ".");
}

XGSEmu::~XGSEmu() {
    // Clean up emulator components
    delete cpu;
    delete video;
    
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
    
    // Hauptschleife
    printf("Starte Hauptschleife...\n");
    Uint32 frameStart;
    int frameTime;
    int frameCount = 0;
    
    try {
        while (running) {
            frameStart = SDL_GetTicks();
            
            // Führe einen Frame aus
            handleEvents();
            
            if (cpu != nullptr) {
                update();
            } else {
                // CPU nicht initialisiert - Warte auf Benutzeraktion
                SDL_Delay(100);
            }
            
            render();
            
            frameTime = SDL_GetTicks() - frameStart;
            
            // Frame-Begrenzung
            if (FRAME_DELAY > frameTime) {
                SDL_Delay(FRAME_DELAY - frameTime);
            }
            
            // Debug-Ausgabe alle 100 Frames
            frameCount++;
            //if (frameCount % 100 == 0) {
            //    printf("Hauptschleife läuft: Frame %d\n", frameCount);
            //}
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

void XGSEmu::handleEvents() {
    SDL_Event event;
    
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                running = false;
                break;
                
            case SDL_KEYDOWN:
                // Handle key presses - SDL2 version
                switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE:
                        running = false;
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
                        
                    case SDLK_UP:
                        if (joystickr) *joystickr |= 0x1;
                        break;
                        
                    case SDLK_RIGHT:
                        if (joystickr) *joystickr |= 0x8;
                        break;
                        
                    case SDLK_DOWN:
                        if (joystickr) *joystickr |= 0x2;
                        break;
                        
                    case SDLK_LEFT:
                        if (joystickr) *joystickr |= 0x4;
                        break;
                        
                    case SDLK_LCTRL:
                    case SDLK_RCTRL:
                        if (joystickr) *joystickr |= 0x10;
                        break;
                }
                break;
                
            case SDL_KEYUP:
                // Game controls
                switch (event.key.keysym.sym) {
                    case SDLK_UP:
                        if (joystickr) *joystickr &= ~0x1;
                        break;
                        
                    case SDLK_RIGHT:
                        if (joystickr) *joystickr &= ~0x8;
                        break;
                        
                    case SDLK_DOWN:
                        if (joystickr) *joystickr &= ~0x2;
                        break;
                        
                    case SDLK_LEFT:
                        if (joystickr) *joystickr &= ~0x4;
                        break;
                        
                    case SDLK_LCTRL:
                    case SDLK_RCTRL:
                        if (joystickr) *joystickr &= ~0x10;
                        break;
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
    if (cpu != nullptr) {
        // Execute CPU instructions
        for (int i = 0; i < INSTRUCTIONS_PER_FRAME; i++) {
            cpu->execute();
        }
        
        // Update texture from emuDisplay
        updateTexture();
    }
}
void XGSEmu::updateTexture() {
    if (emuDisplay != nullptr) {
        // Bei existierender Textur, nur aktualisieren statt neu erstellen
        if (texture != NULL) {
            // Aktualisiere Textur anstatt sie jedes Mal neu zu erstellen
            SDL_UpdateTexture(texture, NULL, emuDisplay->pixels, emuDisplay->pitch);
        } else {
            // Create a texture from the surface
            texture = SDL_CreateTextureFromSurface(renderer, emuDisplay);
            
            if (texture == NULL) {
                printf("Textur konnte nicht erstellt werden! SDL-Fehler: %s\n", SDL_GetError());
                printf("emuDisplay Info: %dx%d, %d bpp\n", 
                       emuDisplay->w, emuDisplay->h, 
                       emuDisplay->format ? emuDisplay->format->BitsPerPixel : 0);
            }
        }
    } else {
        printf("emuDisplay ist NULL! Kann keine Textur erstellen.\n");
    }
}

void XGSEmu::render() {
    if (renderer && texture) {
        // Clear renderer
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        
        // Get current window size
        int windowWidth, windowHeight;
        SDL_GetWindowSize(window, &windowWidth, &windowHeight);
        
        // Calculate scaled dimensions while maintaining aspect ratio
        int textureWidth, textureHeight;
        SDL_QueryTexture(texture, NULL, NULL, &textureWidth, &textureHeight);
        
        float sourceAspect = (float)textureWidth / textureHeight;
        float windowAspect = (float)windowWidth / windowHeight;
        
        int renderWidth, renderHeight;
        
        if (windowAspect > sourceAspect) {
            // Window is wider than source
            renderHeight = windowHeight;
            renderWidth = (int)(windowHeight * sourceAspect);
        } else {
            // Window is taller than source
            renderWidth = windowWidth;
            renderHeight = (int)(windowWidth / sourceAspect);
        }
        
        // Center the texture in the window
        SDL_Rect destRect;
        destRect.x = (windowWidth - renderWidth) / 2;
        destRect.y = (windowHeight - renderHeight) / 2;
        destRect.w = renderWidth;
        destRect.h = renderHeight;
        
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
    
    // Clean up existing components
    if (texture != NULL) {
        SDL_DestroyTexture(texture);
        texture = NULL;
    }
    
    if (video != nullptr) {
        delete video;
        video = nullptr;
    }
    
    // Create new video system
    video = new Video();
    if (video == nullptr) {
        printf("Fehler: Video-System konnte nicht erstellt werden!\n");
        return;
    }
    
    emuDisplay = video->getScreen();
    if (emuDisplay == nullptr) {
        printf("Fehler: Video-Surface konnte nicht erstellt werden!\n");
        return;
    }
    
    printf("Video-Surface erstellt: %dx%d, %d bpp\n", 
           emuDisplay->w, emuDisplay->h, 
           emuDisplay->format ? emuDisplay->format->BitsPerPixel : 0);
    
    // Clean up existing CPU
    if (cpu != nullptr) {
        delete cpu;
        cpu = nullptr;
    }
    
    // Create new CPU and load file
    try {
        cpu = new Cpu(video);
        joystickr = cpu->getJoystrickr();
        
        FileHandler fh;
        printf("Lade ROM-Datei: %s\n", path.c_str());
        if (!fh.readFile(path.c_str(), cpu->getROM())) {
            printf("Fehler beim Lesen der ROM-Datei!\n");
            return;
        }
        
        printf("ROM-Datei erfolgreich geladen.\n");
        
        // Create new texture from the emulator display - NACH dem ROM-Laden
        updateTexture();
        
        // Save directory in config
        size_t lastSlash = path.find_last_of("/\\");
        if (lastSlash != std::string::npos) {
            lastDirectory = path.substr(0, lastSlash);
            saveConfig("LoadDirectory", lastDirectory);
        }
    } catch (std::exception& e) {
        printf("Exception beim Laden der HEX-Datei: %s\n", e.what());
    } catch (...) {
        printf("Unbekannte Exception beim Laden der HEX-Datei!\n");
    }
}

void XGSEmu::reset() {
    if (cpu != nullptr) {
        cpu->reset();
    }
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

void XGSEmu::saveConfig(const std::string& key, const std::string& value) {
    std::string configDir = getConfigDir();
    std::string configFile = configDir + "/config.ini";
    
    // Read existing config
    std::map<std::string, std::string> config;
    std::ifstream inFile(configFile);
    std::string line;
    
    while (std::getline(inFile, line)) {
        size_t equalPos = line.find('=');
        if (equalPos != std::string::npos) {
            std::string k = line.substr(0, equalPos);
            std::string v = line.substr(equalPos + 1);
            config[k] = v;
        }
    }
    inFile.close();
    
    // Update config
    config[key] = value;
    
    // Write updated config
    std::ofstream outFile(configFile);
    for (const auto& pair : config) {
        outFile << pair.first << "=" << pair.second << std::endl;
    }
    outFile.close();
}

std::string XGSEmu::loadConfig(const std::string& key, const std::string& defaultValue) {
    std::string configDir = getConfigDir();
    std::string configFile = configDir + "/config.ini";
    
    std::ifstream inFile(configFile);
    if (!inFile.good()) {
        return defaultValue;
    }
    
    std::string line;
    while (std::getline(inFile, line)) {
        size_t equalPos = line.find('=');
        if (equalPos != std::string::npos) {
            std::string k = line.substr(0, equalPos);
            if (k == key) {
                return line.substr(equalPos + 1);
            }
        }
    }
    
    return defaultValue;
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
    
    // Wenn es Dateien gibt, lade die erste (in einer realen Implementierung würden wir den Benutzer wählen lassen)
    if (!files.empty()) {
        std::string fullPath = lastDirectory + "/" + files[0];
        loadHexFile(fullPath);
        printf("Datei geladen: %s\n", fullPath.c_str());
    } else {
        printf("Keine .hex oder .obj Dateien im Verzeichnis gefunden: %s\n", lastDirectory.c_str());
    }
}
