/*
* =====================================================================================
* 
*        Filename:  filehandler.cpp
* 
*     Description:  adapts to sx_t
* 
*         Version:  1.0
*         Created:  14.07.2005 22:55:31 CEST
*        Revision:  none
*        Compiler:  gcc
* 
*          Author:  Rainer Blessing (), rainer@theblessing.net
*         Company:  
* 
* =====================================================================================
*/

#include "filehandler.h"
#include "objfile.h"
#include <cstring>
#include <strings.h>

// Cross-platform case-insensitive string comparison
int str_case_compare(const char* s1, const char* s2) {
#ifdef _WIN32
    return _stricmp(s1, s2);
#else
    return strcasecmp(s1, s2);
#endif
}

/**
 * Delegates the load requests to the specific load functions
 * @param fn Filename
 * @param rom Pointer to XGSs ROM
 * @return 
 */
bool FileHandler::readFile(const char *fn, u16 * rom, std::string * error)
{
    u16 fusex;
    return readFile(fn, rom, &fusex, error);
}

bool FileHandler::readFile(const char *fn, u16 * rom, u16 * fusex, std::string * error)
{
    size_t fn_length = strlen(fn);
    // the extension, with the dot
    const char* extension = fn_length >= 4 ? fn + fn_length - 4 : "";

    sx_t sx = sx_t();
    if (str_case_compare(".hex", extension) == 0)
    {
        if (!HexFile::readFile(fn, &sx, error)) {
            return false;
        }
    }
    else if (str_case_compare(".obj", extension) == 0)
    {
        ObjFile of;
        if (!of.readFile(fn, &sx, error)) {
            return false;
        }
    }
    else
    {
        if (error != NULL) {
            *error = "not a .hex or .obj file";
        }
        return false;
    }
    memcpy(rom, sx.rom, SX_MAX_ROM_SIZE * sizeof(u16));
    *fusex = sx.rom[FUSEX_LOC];
    return true;
}
