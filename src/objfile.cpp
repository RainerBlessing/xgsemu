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
#include <stdio.h>
#include "objfile.h"

ObjFile::ObjFile()
{}


ObjFile::~ObjFile()
{}

// A binary image of the program memory: two bytes per word, low byte first.
bool ObjFile::readFile(const char *fn, sx_t * sx)
{
	FILE *f = fopen(fn, "rb");
	if (!f)
	{
		perror(fn);
		return false;
	}

	sx->rom[FUSEX_LOC] = 0xfff;
	sx->rom[FUSE_LOC] = 0x06b;

	for (int i = 0; i < SX_MAX_ROM_SIZE; i++)
		sx->rom[i] = 0xfff;

	for (int i = 0; i < SX_ID_SIZE; i++)
		sx->rom[ID_LOC + i] = 0xfff;

	u8 image[SX_MAX_ROM_SIZE * 2];
	size_t bytes = fread(image, sizeof(u8), sizeof(image), f);
	bool readFailed = ferror(f) != 0;
	fclose(f);
	if (readFailed)
	{
		perror(fn);
		return false;
	}
	for (size_t word = 0; word * 2 < bytes; word++)
	{
		// a missing high byte (odd file size) stays as in the erased word
		u16 high = (word * 2 + 1 < bytes) ? image[word * 2 + 1] : 0x0f;
		// the words of the SX have 12 bits
		sx->rom[word] = (image[word * 2] | (high << 8)) & 0x0fff;
	}
	return true;
}

bool ObjFile::writeFile(const char *fn, sx_t * sx)
{
	FILE *f = fopen(fn, "wb");
	if (!f)
	{
		perror(fn);
		return false;
	}

	u8 image[SX_MAX_ROM_SIZE * 2];
	for (int i = 0; i < SX_MAX_ROM_SIZE; i++)
	{
		image[i * 2] = sx->rom[i] & 0xff;
		image[i * 2 + 1] = sx->rom[i] >> 8;
	}
	bool written = fwrite(image, sizeof(u8), sizeof(image), f) == sizeof(image);
	// closing writes what is still buffered, it can fail as well
	return fclose(f) == 0 && written;
}
