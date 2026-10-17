// sxdis [-n] file.hex: lists the programmed ROM words of an Intel HEX file.
// Targets of jmp and call get labels (L followed by the address); -n leaves
// the plain numbers.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include "disasm.h"
#include "hex_file.h"

static sx_t sx;

static bool programmed(int address)
{
	return sx.rom[address] != 0xfff;
}

static std::string labelName(int address)
{
	char name[8];
	snprintf(name, sizeof(name), "L%03x", address);
	return name;
}

// The address a jmp or call at `address` goes to, or -1 if the instruction is
// no jump or the target holds no code. The page is the one set by the last
// "page n" in front of it, otherwise the page the jump itself is in.
static std::map<int, int> findTargets()
{
	std::map<int, int> targets;
	int page = -1;
	for (int address = 0; address < SX_MAX_ROM_SIZE; address++)
	{
		if (!programmed(address))
			continue;
		unsigned short opcode = sx.rom[address] & 0xfff;
		if (opcode >> 3 == 2)
		{
			page = opcode & 7;
			continue;
		}
		bool jmp = opcode >> 9 == 5;
		bool call = opcode >> 8 == 9;
		if (!jmp && !call)
			continue;
		int target = (page >= 0 ? page : address >> 9) << 9 | (opcode & (jmp ? 0x1ff : 0xff));
		page = -1;
		if (programmed(target))
			targets[address] = target;
	}
	return targets;
}

int main(int argc, char** argv)
{
	bool labels = true;
	if (argc == 3 && strcmp(argv[1], "-n") == 0)
	{
		labels = false;
		argv++;
		argc--;
	}
	if (argc != 2 || argv[1][0] == '-')
	{
		std::cerr << "usage: sxdis [-n] file.hex\n";
		return 2;
	}
	if (!HexFile::readFile(argv[1], &sx))
		return 1;

	std::map<int, int> targets;
	std::map<int, bool> labelled;
	if (labels)
	{
		targets = findTargets();
		for (std::map<int, int>::const_iterator t = targets.begin(); t != targets.end(); ++t)
			labelled[t->second] = true;
	}
	for (int address = 0; address < SX_MAX_ROM_SIZE; address++)
	{
		if (!programmed(address))
			continue;
		if (labelled.count(address))
			printf("%s:\n", labelName(address).c_str());
		std::string text = disassemble(sx.rom[address]);
		if (targets.count(address))
			text = text.substr(0, text.find(' ') + 1) + labelName(targets[address]);
		printf("%03x: %03x  %s\n", address, sx.rom[address] & 0xfff, text.c_str());
	}
	return 0;
}
