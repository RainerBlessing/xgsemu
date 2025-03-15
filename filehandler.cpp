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
//#include <strings.h>
/**
 * Delegates the load requests to the specific load functions
 * @param fn Filename
 * @param rom Pointer to XGSs ROM
 * @return 
 */
bool FileHandler::readFile(const char *fn, u16 * rom )
{
	sx_t sx;
	if (stricmp("hex", fn + strlen(fn) - 3) == 0)
	{
		HexFile::readFile(fn, &sx);
	}
	else if (stricmp("obj", fn + strlen(fn) - 3) == 0)
	{
		ObjFile of;
		of.readFile(fn, &sx);
	}
	memcpy(rom, sx.rom, 4096*2);
	return true;
}

