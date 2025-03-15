/*
* SxKey.h, part of gSXprog
*
* Copyright 2001-2003 Eric Smith <eric@brouhaha.com>
*
* $Id: SxKey.h,v 1.5 2003/11/18 13:59:42 garlick Exp $
*/

#ifndef SXKEY_H
#define SXKEY_H

#include <string>

#define SX_MAX_ROM_SIZE 	4096
#define SX_ID_SIZE 		16

/* hex file offsets */
#define ID_LOC 			0x1000
#define FUSE_LOC 		0x1010
#define FUSEX_LOC 		0x1011


#define ARRAY_SIZE 		(SX_MAX_ROM_SIZE + SX_ID_SIZE + 2 + 256)
typedef struct
{
	u16 device;
	u16 rom[ ARRAY_SIZE ];
}
sx_t;


#endif

/*
 * em:Local Variables: 
 * em:indent-tabs-mode:nil
 * em:c-basic-offset:4
 * em:End:
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
