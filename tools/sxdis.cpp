// sxdis file.hex: lists the programmed ROM words of an Intel HEX file.
#include <cstdio>
#include <iostream>
#include "disasm.h"
#include "hex_file.h"

static sx_t sx;

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		std::cerr << "usage: sxdis file.hex\n";
		return 2;
	}
	if (!HexFile::readFile(argv[1], &sx))
		return 1;
	for (int address = 0; address < SX_MAX_ROM_SIZE; address++)
	{
		if (sx.rom[address] == 0xfff)
			continue;
		printf("%03x: %03x  %s\n", address, sx.rom[address] & 0xfff, disassemble(sx.rom[address]).c_str());
	}
	return 0;
}
