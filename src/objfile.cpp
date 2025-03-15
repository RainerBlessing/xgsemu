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

bool ObjFile::readFile(const char *fn, sx_t * sx)
{
	bool r = 0;
	FILE *f = NULL;
	int i;

	f = fopen(fn, "r");
	if (!f)
	{
		perror(fn);
		goto error;
	}

	sx->rom[FUSEX_LOC] = 0xfff;
	sx->rom[FUSE_LOC] = 0x06b;

	for (i = 0; i < SX_MAX_ROM_SIZE; i++)
		sx->rom[i] = 0xfff;

	for (i = 0; i < SX_ID_SIZE; i++)
		sx->rom[ID_LOC + i] = 0xfff;
	i = 0;
	i = fread(sx->rom, sizeof(char), SX_MAX_ROM_SIZE * 2, f);
	r = 1;
error:
	if (f)
		fclose(f);
	return (r);
}

bool ObjFile::writeFile( char *fn, sx_t * sx )
{
	FILE *f = NULL;
	int r = 0;
	int i;
	
	f = fopen(fn, "w");
	if (!f)
	{
		perror(fn);
		goto error;
	}
	for (i = 0; i < SX_MAX_ROM_SIZE; i += 8)
	{}

	r = 1;

error:
	if (f)
		fclose(f);
	return (r);

}
