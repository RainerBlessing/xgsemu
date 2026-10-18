/*
* hex_file.c, part of gSXprog
*
* Copyright 2001-2003 Eric Smith <eric@brouhaha.com>
*
* $Id: hex_file.c,v 1.5 2003/11/18 13:59:42 garlick Exp $
*/


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

std::string lineError( int number, const char * message )
{
	char text[ 32 ];
	snprintf( text, sizeof( text ), "line %d: ", number );
	return std::string( text ) + message;
}

struct FileCloser
{
	void operator() ( FILE * f ) const
	{
		fclose( f );
	}
};
}

int HexFile::parse_hex_digit( char **p )
{
	int r = -1;
	if ( ( ( **p ) >= '0' ) && ( ( **p ) <= '9' ) )
		r = **p - '0';
	else if ( ( ( **p ) >= 'A' ) && ( ( **p ) <= 'F' ) )
		r = 10 + **p - 'A';
	else if ( ( ( **p ) >= 'a' ) && ( ( **p ) <= 'f' ) )
		r = 10 + **p - 'a';
	if ( r >= 0 )
		( *p ) ++;
	return ( r );
}

int HexFile::parse_hex_digits( char **p, int digits, u8 * checksum )
{
	int r = 0;
	int d;
	while ( digits-- )
	{
		d = parse_hex_digit( p );
		if ( d < 0 )
			return ( -1 );
		r = ( r << 4 ) + d;
		if ( checksum && !( digits & 1 ) )
			( *checksum ) += ( r & 0xff );
	}
	return ( r );
}

bool HexFile::readFile(const char *fn, sx_t * sx, std::string * error )
{
	FILE *f = fopen( fn, "r" );
	if ( !f )
		return fail( error, strerror( errno ) );
	std::unique_ptr<FILE, FileCloser> file( f );

	// a record has at most 255 data bytes: ':', 2 + 4 + 2 + 510 + 2 digits, a line ending and the 0
	char buf[ 600 ];

	sx->rom[ FUSEX_LOC ] = 0xfff;
	sx->rom[ FUSE_LOC ] = 0xfff;

	for ( int i = 0; i < SX_MAX_ROM_SIZE; i++ )
		sx->rom[ i ] = 0xfff;

	for ( int i = 0; i < SX_ID_SIZE; i++ )
		sx->rom[ ID_LOC + i ] = 0xfff;

	int lineNumber = 0;
	while ( fgets( &buf[ 0 ], sizeof( buf ), f ) )
	{
		lineNumber++;
		size_t length = strlen( buf );
		if ( length == sizeof( buf ) - 1 && buf[ length - 1 ] != '\n' )
		{
			int next = fgetc( f );
			if ( next != EOF )
				return fail( error, lineError( lineNumber, "line too long" ) );
		}
		u8 checksum = 0;
		char *p = &buf[ 0 ];
		while ( ( *p ) && ( ( *p ) != ':' ) )
			p++;
		if ( *p != ':' )
			continue;           /* a line that is no record is ignored */
		p++;
		int byte_count = parse_hex_digits( &p, 2, &checksum );
		if ( byte_count < 0 )
			return fail( error, lineError( lineNumber, "bogus byte count" ) );
		int address = parse_hex_digits( &p, 4, &checksum );
		if ( address < 0 )
			return fail( error, lineError( lineNumber, "bogus address" ) );
		int record_type = parse_hex_digits( &p, 2, &checksum );
		if ( record_type < 0 )
			return fail( error, lineError( lineNumber, "bogus record type" ) );
		if ( record_type == 2 || record_type == 4 )
		{
			/* The extended address records only say "address 0", as gpasm writes them. Any
			   other address would put the data somewhere else. */
			for ( int i = 0; i < byte_count; i++ )
			{
				int byte = parse_hex_digits( &p, 2, &checksum );
				if ( byte < 0 )
					return fail( error, lineError( lineNumber, "bogus data byte" ) );
				if ( byte != 0 )
					return fail( error, lineError( lineNumber, "extended addresses are not supported" ) );
			}
			continue;
		}
		if ( record_type > 1 )
			continue;           /* the other record types are ignored */
		if ( ( record_type == 1 ) && ( byte_count != 0 ) )
			return fail( error, lineError( lineNumber, "end record with data bytes" ) );
		for ( int i = 0; i < byte_count; i++ )
		{
			int byte = parse_hex_digits( &p, 2, &checksum );
			if ( byte < 0 )
				return fail( error, lineError( lineNumber, "bogus data byte" ) );
			/* behind the fuse words the SX-Key writes more words of its own */
			if ( ( address + i ) >= ( ARRAY_SIZE * 2 ) )
			{
				char message[ 48 ];
				snprintf( message, sizeof( message ), "address %04x is out of bounds", address + i );
				return fail( error, lineError( lineNumber, message ) );
			}
			if ( ( address + i ) & 1 )
				sx->rom[ ( address + i ) / 2 ] =
				    ( sx->rom[ ( address + i ) / 2 ] & 0x00ff )
				    + ( byte << 8 );
			else
				sx->rom[ ( address + i ) / 2 ] =
				    ( sx->rom[ ( address + i ) / 2 ] & 0xff00 )
				    + ( byte );
		}
		int byte = parse_hex_digits( &p, 2, &checksum );
		if ( byte < 0 )
			return fail( error, lineError( lineNumber, "bogus checksum" ) );
		if ( checksum != 0 )
			return fail( error, lineError( lineNumber, "incorrect checksum" ) );
		if ( record_type == 1 )
			break;              /* logical end of file */
	}

	if ( ferror( f ) )
		return fail( error, strerror( errno ) );

	/* the words of the SX have 12 bits */
	for ( int i = 0; i <= FUSEX_LOC; i++ )
		sx->rom[ i ] &= 0x0fff;

	return true;
}


void HexFile::writeByte( FILE * f, u8 byte, u8 * checksum )
{
	fprintf( f, "%02X", byte );
	if ( checksum )
		*checksum += byte;
}

bool HexFile::writeFile( const char *fn, sx_t * sx, std::string * error )
{
	int i, j;
	u8 checksum;

	FILE *f = fopen( fn, "w" );
	if ( !f )
		return fail( error, strerror( errno ) );
	for ( i = 0; i < SX_MAX_ROM_SIZE; i += 8 )
	{
		checksum = 0;
		fprintf( f, ":" );
		writeByte( f, 0x10, &checksum );     /* byte count */
		writeByte( f, ( i * 2 ) >> 8, &checksum );     /* address high */
		writeByte( f, ( i * 2 ) & 0xff, &checksum );   /* address low */
		writeByte( f, 0x00, &checksum );     /* record type */
		for ( j = i; j < ( i + 8 ); j++ )
		{
			writeByte( f, sx->rom[ j ] & 0xff, &checksum );
			writeByte( f, sx->rom[ j ] >> 8, &checksum );
		}
		writeByte( f, 256 - checksum, NULL );
		fprintf( f, "\n" );
	}
	for ( i = ID_LOC; i < ID_LOC + SX_ID_SIZE; i += 8 )
	{
		checksum = 0;
		fprintf( f, ":" );
		writeByte( f, 0x10, &checksum );     /* byte count */
		writeByte( f, ( i * 2 ) >> 8, &checksum );     /* address high */
		writeByte( f, ( i * 2 ) & 0xff, &checksum );   /* address low */
		writeByte( f, 0x00, &checksum );     /* record type */
		for ( j = i; j < ( i + 8 ); j++ )
		{
			writeByte( f, sx->rom[ j ] & 0xff, &checksum );
			writeByte( f, sx->rom[ j ] >> 8, &checksum );
		}
		writeByte( f, 256 - checksum, NULL );
		fprintf( f, "\n" );
	}

	checksum = 0;
	fprintf( f, ":" );
	writeByte( f, 0x04, &checksum ); /* byte count */
	writeByte( f, ( FUSE_LOC * 2 ) >> 8, &checksum );  /* address high */
	writeByte( f, ( FUSE_LOC * 2 ) & 0xff, &checksum );        /* address low */
	writeByte( f, 0x00, &checksum ); /* record type */
	for ( j = FUSE_LOC; j <= FUSEX_LOC; j++ )
	{
		writeByte( f, sx->rom[ j ] & 0xff, &checksum );
		writeByte( f, sx->rom[ j ] >> 8, &checksum );
	}
	writeByte( f, 256 - checksum, NULL );
	fprintf( f, "\n" );

	checksum = 0;
	fprintf( f, ":" );
	writeByte( f, 0x00, &checksum ); /* byte count */
	writeByte( f, 0x00, &checksum ); /* address high */
	writeByte( f, 0x00, &checksum ); /* address low */
	writeByte( f, 0x01, &checksum ); /* record type */
	writeByte( f, 256 - checksum, NULL );
	fprintf( f, "\n" );

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
