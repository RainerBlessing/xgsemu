// sxasm source.asm output.hex: assembles SX52 source into an Intel HEX file.
#include <fstream>
#include <iostream>
#include <sstream>
#include "asm.h"
#include "hex_file.h"

static sx_t sx;

int main(int argc, char** argv)
{
	if (argc != 3)
	{
		std::cerr << "usage: sxasm source.asm output.hex\n";
		return 2;
	}
	std::ifstream file(argv[1]);
	if (!file)
	{
		std::cerr << argv[1] << ": cannot open\n";
		return 1;
	}
	std::stringstream source;
	source << file.rdbuf();

	// ID and fuses stay unprogrammed (0xfff)
	for (int i = 0; i < ARRAY_SIZE; i++)
		sx.rom[i] = 0xfff;
	std::string error;
	if (!assembleSource(source.str(), sx.rom, SX_MAX_ROM_SIZE, &error))
	{
		std::cerr << argv[1] << ": " << error << "\n";
		return 1;
	}
	return HexFile::writeFile(argv[2], &sx) ? 0 : 1;
}
