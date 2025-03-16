// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#ifndef STDAFX_H
#define STDAFX_H

#ifdef _WIN32
// Windows specific headers
#include <windows.h>
#else
// Linux/Unix specific headers (if needed)
#include <unistd.h>
#endif

// Common headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// C++ headers
#include <string>
#include <iostream>
#include <sstream>

// SDL headers
#include "SDL.h"

// wxWidgets headers
#include "wx/wx.h"

#endif // STDAFX_H
