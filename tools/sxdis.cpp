// sxdis [-n] [-s] file.hex: lists the programmed ROM words of an Intel HEX file.
// Targets of jmp and call get labels (L followed by the address); -n leaves
// the plain numbers. -s writes source for sxasm instead of a listing.
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

// Words that are no instruction (the disassembler says "not impl") are data.
static std::string sourceText(int address, const std::string& text)
{
	if (text.compare(0, 8, "not impl") == 0)
	{
		char data[16];
		snprintf(data, sizeof(data), "dw %x", sx.rom[address] & 0xfff);
		return data;
	}
	return text;
}

int main(int argc, char** argv)
{
	bool labels = true;
	bool source = false;
	int i = 1;
	for (; i < argc && argv[i][0] == '-'; i++)
	{
		if (strcmp(argv[i], "-n") == 0)
			labels = false;
		else if (strcmp(argv[i], "-s") == 0)
			source = true;
		else
			break;
	}
	if (argc - i != 1 || argv[i][0] == '-')
	{
		std::cerr << "usage: sxdis [-n] [-s] file.hex\n";
		return 2;
	}
	std::string error;
	if (!HexFile::readFile(argv[i], &sx, &error))
	{
		std::cerr << argv[i] << ": " << error << "\n";
		return 1;
	}

	std::map<int, int> targets;
	std::map<int, bool> labelled;
	if (labels)
	{
		targets = findTargets();
		for (std::map<int, int>::const_iterator t = targets.begin(); t != targets.end(); ++t)
			labelled[t->second] = true;
	}
	int next = 0;		// the address the assembler is at
	for (int address = 0; address < SX_MAX_ROM_SIZE; address++)
	{
		if (!programmed(address))
			continue;
		std::string text = disassemble(sx.rom[address]);
		if (targets.count(address))
			text = text.substr(0, text.find(' ') + 1) + labelName(targets[address]);
		// the org comes first: a label is bound to the address at its line
		if (source && address != next)
			printf("        org %x\n", address);
		if (labelled.count(address))
			printf("%s:\n", labelName(address).c_str());
		if (source)
			printf("        %s\n", sourceText(address, text).c_str());
		else
			printf("%03x: %03x  %s\n", address, sx.rom[address] & 0xfff, text.c_str());
		next = address + 1;
	}
	return 0;
}
