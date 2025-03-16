# FindSDL.cmake
#
# This module defines:
#  SDL_INCLUDE_DIRS - where to find SDL.h
#  SDL_LIBRARIES - library to link against to use SDL
#  SDL_FOUND - true if SDL has been found

# Find SDL include directory
find_path(SDL_INCLUDE_DIR SDL.h
  HINTS
    ENV SDLDIR
  PATH_SUFFIXES SDL SDL1 include include/SDL
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local/include
  /usr/include
  /sw/include
  /opt/local/include
  /opt/csw/include
  /opt/include
)

# Find SDL library
find_library(SDL_LIBRARY
  NAMES SDL SDL-1.1 SDL-1.2
  HINTS
    ENV SDLDIR
  PATH_SUFFIXES lib lib64
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local
  /usr
  /sw
  /opt/local
  /opt/csw
  /opt
)

# Find SDL_main library (Windows specific)
find_library(SDL_MAIN_LIBRARY
  NAMES SDLmain
  HINTS
    ENV SDLDIR
  PATH_SUFFIXES lib lib64
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local
  /usr
  /sw
  /opt/local
  /opt/csw
  /opt
)

# Handle the QUIETLY and REQUIRED arguments
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SDL DEFAULT_MSG SDL_LIBRARY SDL_INCLUDE_DIR)

# Set output variables
if(SDL_FOUND)
  set(SDL_LIBRARIES ${SDL_LIBRARY})
  # Add SDLmain if found and we're on Windows
  if(WIN32 AND SDL_MAIN_LIBRARY)
    list(APPEND SDL_LIBRARIES ${SDL_MAIN_LIBRARY})
  endif()
  set(SDL_INCLUDE_DIRS ${SDL_INCLUDE_DIR})
endif()

# Hide internal variables
mark_as_advanced(SDL_INCLUDE_DIR SDL_LIBRARY SDL_MAIN_LIBRARY)
