/*
* =====================================================================================
* 
*        Filename:  filehandler.h
* 
*     Description:  adapts to sx_t
* 
*         Version:  1.0
*         Created:  14.07.2005 22:53:03 CEST
*        Revision:  none
*        Compiler:  gcc
* 
*          Author:  Rainer Blessing 
*         Company:  
*           Email:  rainer@theblessing.net
* 
* =====================================================================================
*/
#ifndef SIMHEXFILE_H
#define SIMHEXFILE_H

#include "hex_file.h"

class FileHandler
{
public:
	/**
	 * Reads the program of a .hex or .obj file into the 4096 words of ROM.
	 * @param error NULL, or receives the reason if the file is refused
	 */
	static bool readFile(const char *fn, u16 * sx, std::string * error = NULL);
	/**
	 * Reads the program and the FUSEX word of the file. The CF bit (bit 6) of it decides
	 * whether the carry flag is an input of ADD and SUB.
	 */
	static bool readFile(const char *fn, u16 * sx, u16 * fusex, std::string * error = NULL);
};
#endif
