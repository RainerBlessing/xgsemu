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
	/**
	 * Reads the words of a hex file: ROM, ID and fuses. The words have 12 bits. Lines that are no
	 * records and record types that are not understood are ignored, so is a missing end record.
	 * @param error NULL, or receives the reason (with the line number) if the file is refused
	 * @return false if the file cannot be read or is broken
	 */
	static bool readFile(const char *fn, sx_t * sx, std::string * error = NULL);
	/** @param error NULL, or receives the reason if the file cannot be written */
	static bool writeFile(const char *fn, sx_t * sx, std::string * error = NULL);
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
