/*
* hex_file.h, part of gSXprog
*
* Copyright 2001-2003 Eric Smith <eric@brouhaha.com>
*
* $Id: hex_file.h,v 1.4 2003/11/18 13:59:42 garlick Exp $
*/

#ifndef HEXFILE_H
#define HEXFILE_H

#include <stdio.h>
#include <string>
#include "sxkey.h"

/**
 * Handles Hex files
 */
class HexFile
{
public:
	HexFile() {}
	static bool readFile(const char *fn, sx_t * sx);
	static bool writeFile(char *fn, sx_t * sx);
private:
	static void writeByte(FILE * f, u8 byte, u8 * checksum);
	static int parse_hex_digit(char**);
	static int parse_hex_digits(char **p, int digits, u8 * checksum);
};

#endif

/*
 * em:Local Variables: 
 * em:indent-tabs-mode:nil
 * em:c-basic-offset:4
 * em:End:
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
