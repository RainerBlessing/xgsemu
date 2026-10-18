/*
* hex_file.c, part of gSXprog
*
* Copyright 2001-2003 Eric Smith <eric@brouhaha.com>
*
* $Id: hex_file.c,v 1.5 2003/11/18 13:59:42 garlick Exp $
*/


#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <memory>
#include "hex_file.h"

namespace
{
bool fail( std::string * error, const std::string & message )
{
	if ( error != NULL )
		*error = message;
	return false;
}

bool fail( std::string * error, int lineNumber, const char * message )
{
	char text[ 32 ];
	snprintf( text, sizeof( text ), "line %d: ", lineNumber );
	return fail( error, std::string( text ) + message );
}

struct FileCloser
{
	void operator() ( FILE * f ) const
	{
		fclose( f );
	}
};

/* The hex digits of one line, and the checksum of the bytes that were read from it. */
class Digits
{
public:
	explicit Digits( char * text ) : p( text ), checksum( 0 ) {}

	/* A number of "count" digits, or -1 if there is a character that is no hex digit. */
	int read( int count )
	{
		int r = 0;
		while ( count-- )
		{
			int d = digit();
			if ( d < 0 )
				return -1;
			r = ( r << 4 ) + d;
			if ( !( count & 1 ) )      /* a byte is complete */
				checksum += ( r & 0xff );
		}
		return r;
	}

	bool checksumIsRight() const
	{
		return checksum == 0;
	}

private:
	int digit()
	{
		static const char HEX[] = "0123456789abcdef";
		const char * found = *p ? strchr( HEX, tolower( (unsigned char) *p ) ) : NULL;
		if ( found == NULL )
			return -1;
		p++;
		return (int)( found - HEX );
	}

	char * p;
	u8 checksum;
};

struct Header
{
	int byteCount;
	int address;
	int type;
};

const int DATA_RECORD = 0;
const int END_RECORD = 1;
const int EXTENDED_SEGMENT_RECORD = 2;
const int EXTENDED_LINEAR_RECORD = 4;

bool readHeader( Digits & in, Header * header, int lineNumber, std::string * error )
{
	header->byteCount = in.read( 2 );
	if ( header->byteCount < 0 )
		return fail( error, lineNumber, "bogus byte count" );
	header->address = in.read( 4 );
	if ( header->address < 0 )
		return fail( error, lineNumber, "bogus address" );
	header->type = in.read( 2 );
	if ( header->type < 0 )
		return fail( error, lineNumber, "bogus record type" );
	return true;
}

/* The extended address records only say "address 0", as gpasm writes them. Any other address
   would put the data somewhere else. */
bool checkExtendedAddress( Digits & in, const Header & header, int lineNumber, std::string * error )
{
	for ( int i = 0; i < header.byteCount; i++ )
	{
		int byte = in.read( 2 );
		if ( byte < 0 )
			return fail( error, lineNumber, "bogus data byte" );
		if ( byte != 0 )
			return fail( error, lineNumber, "extended addresses are not supported" );
	}
	return true;
}

/* The bytes of a data record: the low byte of a word comes first. */
bool storeData( Digits & in, const Header & header, sx_t * sx, int lineNumber, std::string * error )
{
	for ( int i = 0; i < header.byteCount; i++ )
	{
		int byte = in.read( 2 );
		if ( byte < 0 )
			return fail( error, lineNumber, "bogus data byte" );
		int address = header.address + i;
		/* behind the fuse words the SX-Key writes more words of its own */
		if ( address >= ( ARRAY_SIZE * 2 ) )
		{
			char message[ 48 ];
			snprintf( message, sizeof( message ), "address %04x is out of bounds", address );
			return fail( error, lineNumber, message );
		}
		if ( address & 1 )
			sx->rom[ address / 2 ] = ( sx->rom[ address / 2 ] & 0x00ff ) + ( byte << 8 );
		else
			sx->rom[ address / 2 ] = ( sx->rom[ address / 2 ] & 0xff00 ) + ( byte );
	}
	return true;
}

/* The last byte of a record makes the sum of all of its bytes 0. */
bool checkChecksum( Digits & in, int lineNumber, std::string * error )
{
	if ( in.read( 2 ) < 0 )
		return fail( error, lineNumber, "bogus checksum" );
	if ( !in.checksumIsRight() )
		return fail( error, lineNumber, "incorrect checksum" );
	return true;
}

/* The first digit of the record of a line (behind the colon), or NULL for a line that is no record. */
char * recordStart( char * line )
{
	char * colon = strchr( line, ':' );
	return colon == NULL ? NULL : colon + 1;
}

bool readEndRecord( Digits & in, const Header & header, int lineNumber, bool * isEnd, std::string * error )
{
	if ( header.byteCount != 0 )
		return fail( error, lineNumber, "end record with data bytes" );
	*isEnd = true;
	return checkChecksum( in, lineNumber, error );
}

/* What follows the header, by the type of the record. The types that are not understood are ignored. */
bool readBody( Digits & in, const Header & header, sx_t * sx, int lineNumber, bool * isEnd, std::string * error )
{
	switch ( header.type )
	{
	case DATA_RECORD:
		return storeData( in, header, sx, lineNumber, error )
		       && checkChecksum( in, lineNumber, error );
	case END_RECORD:
		return readEndRecord( in, header, lineNumber, isEnd, error );
	case EXTENDED_SEGMENT_RECORD:
	case EXTENDED_LINEAR_RECORD:
		return checkExtendedAddress( in, header, lineNumber, error )
		       && checkChecksum( in, lineNumber, error );
	default:
		return true;
	}
}

/* One line of the file. A line that is no record is ignored. */
bool readRecord( char * line, int lineNumber, sx_t * sx, bool * isEnd, std::string * error )
{
	char * start = recordStart( line );
	if ( start == NULL )
		return true;

	Digits in( start );
	Header header;
	return readHeader( in, &header, lineNumber, error )
	       && readBody( in, header, sx, lineNumber, isEnd, error );
}

void eraseImage( sx_t * sx )
{
	for ( int i = 0; i < SX_MAX_ROM_SIZE; i++ )
		sx->rom[ i ] = 0xfff;
	for ( int i = 0; i < SX_ID_SIZE; i++ )
		sx->rom[ ID_LOC + i ] = 0xfff;
	sx->rom[ FUSE_LOC ] = 0xfff;
	sx->rom[ FUSEX_LOC ] = 0xfff;
}

/* Is this the cut off start of a line that does not fit into the buffer? */
bool lineIsTooLong( const char * line, size_t size, FILE * f )
{
	size_t length = strlen( line );
	return length == size - 1 && line[ length - 1 ] != '\n' && fgetc( f ) != EOF;
}

/* The records of the file up to the end record. *seenEnd tells if there was one. */
bool readRecords( FILE * f, sx_t * sx, bool * seenEnd, std::string * error )
{
	// a record has at most 255 data bytes: ':', 2 + 4 + 2 + 510 + 2 digits, a line ending and the 0
	char buf[ 600 ];
	int lineNumber = 0;
	while ( !*seenEnd && fgets( buf, sizeof( buf ), f ) )
	{
		lineNumber++;
		if ( lineIsTooLong( buf, sizeof( buf ), f ) )
			return fail( error, lineNumber, "line too long" );
		if ( !readRecord( buf, lineNumber, sx, seenEnd, error ) )
			return false;
	}
	return true;
}
}

bool HexFile::readFile(const char *fn, sx_t * sx, std::string * error )
{
	FILE *f = fopen( fn, "r" );
	if ( !f )
		return fail( error, strerror( errno ) );
	std::unique_ptr<FILE, FileCloser> file( f );

	eraseImage( sx );

	bool seenEnd = false;
	if ( !readRecords( f, sx, &seenEnd, error ) )
		return false;
	if ( ferror( f ) )
		return fail( error, strerror( errno ) );
	/* a file that stops early would load as a program with a hole at the end */
	if ( !seenEnd )
		return fail( error, "no end record" );

	/* the words of the SX have 12 bits */
	for ( int i = 0; i <= FUSEX_LOC; i++ )
		sx->rom[ i ] &= 0x0fff;

	return true;
}


namespace
{
/* One record: ':', the byte count, the address, the type, the data and the checksum, each as hex
   digits. */
void writeRecord( FILE * f, int address, int type, const u8 * data, int count )
{
	u8 header[ 4 ] = { (u8) count, (u8)( address >> 8 ), (u8)( address & 0xff ), (u8) type };
	u8 sum = 0;
	fprintf( f, ":" );
	for ( int i = 0; i < 4; i++ )
	{
		fprintf( f, "%02X", header[ i ] );
		sum += header[ i ];
	}
	for ( int i = 0; i < count; i++ )
	{
		fprintf( f, "%02X", data[ i ] );
		sum += data[ i ];
	}
	fprintf( f, "%02X\n", (u8)( 256 - sum ) );
}

/* The words from "first" on, low byte first, in records of up to 8 words (16 bytes). */
void writeWords( FILE * f, const sx_t * sx, int first, int words )
{
	for ( int i = 0; i < words; i += 8 )
	{
		u8 data[ 16 ];
		int count = words - i < 8 ? words - i : 8;
		for ( int j = 0; j < count; j++ )
		{
			data[ 2 * j ] = sx->rom[ first + i + j ] & 0xff;
			data[ 2 * j + 1 ] = sx->rom[ first + i + j ] >> 8;
		}
		writeRecord( f, ( first + i ) * 2, DATA_RECORD, data, count * 2 );
	}
}
}

bool HexFile::writeFile( const char *fn, sx_t * sx, std::string * error )
{
	FILE *f = fopen( fn, "w" );
	if ( !f )
		return fail( error, strerror( errno ) );

	writeWords( f, sx, 0, SX_MAX_ROM_SIZE );
	writeWords( f, sx, ID_LOC, SX_ID_SIZE );
	writeWords( f, sx, FUSE_LOC, FUSEX_LOC - FUSE_LOC + 1 );
	writeRecord( f, 0, END_RECORD, NULL, 0 );

	// closing writes what is still buffered, it can fail as well
	bool written = ferror( f ) == 0;
	int reason = errno;
	if ( fclose( f ) != 0 && written )
	{
		written = false;
		reason = errno;
	}
	if ( !written )
		return fail( error, strerror( reason ) );
	return true;
}

/*
 * em:Local Variables: 
 * em:indent-tabs-mode:nil
 * em:c-basic-offset:4
 * em:End:
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
