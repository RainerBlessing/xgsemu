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

#include <SDL.h>
#include <stdio.h>
#include <string>
#include <fstream>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include "xgsemu.h"

#define XGSE_VERSION "0.07"

// Find application path (simplified version of wxFindAppPath)
std::string findAppPath(const char* argv0) {
    std::string path = argv0;
    size_t lastSlash = path.find_last_of("/\\");
    
    if (lastSlash != std::string::npos) {
        return path.substr(0, lastSlash);
    }
    
    // If no slash found, use current directory
    char currentDir[PATH_MAX];
    if (getcwd(currentDir, PATH_MAX) != NULL) {
        return std::string(currentDir);
    }
    
    return ".";
}

int main(int argc, char* argv[]) {
    // Initialize SDL (for SDL 1.2)
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }
    
    // Get application directory
    std::string appDir = findAppPath(argv[0]);
    
    // Process command line arguments
    std::string hexFilePath = "";
    
    for (int i = 1; i < argc; i++) {
        // If argument doesn't start with "-", treat it as hex file path
        if (argv[i][0] != '-') {
            hexFilePath = argv[i];
            break;
        }
    }
    
    // Create emulator instance
    XGSEmu emulator(appDir, std::string("XGSEmu ") + XGSE_VERSION);
    
    // Start the emulator
    int result = emulator.run(hexFilePath);
    
    // Cleanup SDL
    SDL_Quit();
    
    return result;
}
