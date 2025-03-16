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
bool FileHandler::readFile(const char *fn, u16 * rom)
{
    sx_t sx;
    size_t fn_length = strlen(fn);

    if (fn_length < 3) {
        return false;
    }

    const char* extension = fn + strlen(fn) - 3;
    if (str_case_compare("hex", extension) == 0)
    {
        HexFile::readFile(fn, &sx);
    }
    else if (str_case_compare("obj", extension) == 0)
    {
        ObjFile of;
        of.readFile(fn, &sx);
    }
    memcpy(rom, sx.rom, 4096*2);
    return true;
}
