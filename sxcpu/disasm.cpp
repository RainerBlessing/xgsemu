#include "disasm.h"
#include <sstream>

using namespace std;

string disassemble(unsigned short opc)
{
	string
	op = "";
	ostringstream outs;
	unsigned short
	mne = opc;
	if (mne == 0xfff)
		return "unused";

	if (mne >> 9 == 0x5)
	{				//jmp
		outs << "jmp " << hex << (mne & 0x1ff);
		op = outs.str();
		return op;
	}
	else if (mne >> 8 == 0xc)
	{				//mov w,lit
		outs << "mov w, #" << hex << (mne & 0xff);
		op = outs.str();

	}
	else if (mne >> 5 == 1)
	{				//mov fr,w
		outs << "mov " << hex << (mne & 0x1f) << ", w";
		op = outs.str();

	}
	else if (mne >> 8 == 5)
	{				//setb fr.bit
		int
		bit = (mne >> 5) & 7;
		outs << "setb " << hex << (mne & 0x1f) << "." << bit;
		op = outs.str();

	}
	else if (mne >> 5 == 0xf)
	{				//add fr,w
		outs << "add " << hex << (mne & 0x1f) << ", w";
		op = outs.str();

	}
	else if (mne >> 5 == 0xe)
	{				//add w,fr

		outs << "add w, " << hex << (mne & 0x1f);
		op = outs.str();

	}
	else if (mne >> 8 == 6)
	{				//snb fr.bit
		int
		bit = (mne >> 5) & 7;
		outs << "snb " << hex << (mne & 0x1f) << "." << bit;
		op = outs.str();

	}
	else if (mne >> 5 == 11)
	{				//and fr,w
		outs << "and " << hex << (mne & 0x1f) << ", w";
		op = outs.str();

	}
	else if (mne >> 5 == 10)
	{				//and w,fr
		outs << "and w, " << hex << (mne & 0x1f);
		op = outs.str();

	}
	else if (mne >> 8 == 0xe)
	{				//and w,#lit
		outs << "and w, #" << hex << (mne & 0xff);
		op = outs.str();

	}
	else if (mne >> 5 == 0x13)
	{				//not fr
		outs << "not " << hex << (mne & 0x1f);
		op = outs.str();

	}
	else if (mne >> 5 == 9)
	{				//or fr,w
		outs << "or " << hex << (mne & 0x1f) << ", w";
		op = outs.str();

	}
	else if (mne >> 8 == 0xd)
	{				//or w,#lit
		outs << "or w, #" << hex << (mne & 0xff);
		op = outs.str();

	}
	else if (mne >> 5 == 8)
	{				//or w,fr
		outs << "or w, " << hex << (mne & 0x1f);
		op = outs.str();

	}
	else if (mne >> 5 == 13)
	{				//xor fr,w
		outs << "xor " << hex << (mne & 0x1f) << ", w";
		op = outs.str();

	}
	else if (mne >> 5 == 12)
	{				//xor w,fr
		outs << "xor w, " << hex << (mne & 0x1f);
		op = outs.str();

	}
	else if (mne >> 8 == 0xf)
	{				//xor w,#lit
		outs << "xor w, #" << hex << (mne & 0xff);
		op = outs.str();

	}
	else if (mne >> 5 == 3)
	{				//clr fr
		outs << "clr " << hex << (mne & 0x1f);
		op = outs.str();

	}
	else if (mne == 0x40)
	{				//clr w
		op = "clr w";
	}
	else if (mne == 4)
	{				//clr !WDT
		op = "clr !WDT";
	}
	else if (mne >> 5 == 7)
	{				//dec fr
		outs << "dec " << hex << (mne & 0x1f);
		op = outs.str();

	}
	else if (mne >> 8 == 7)
	{				//sb fr.bit
		int
		bit = (mne >> 5) & 7;
		outs << "sb " << hex << (mne & 0x1f) << "." << bit;
		op = outs.str();

	}
	else if (mne >> 5 == 0x17)
	{				//decsz fr
		outs << "decsz " << hex << (mne & 0x1f);
		op = outs.str();

	}
	else if (mne >> 5 == 0x15)
	{				//inc fr
		outs << "inc " << hex << (mne & 0x1f);
		op = outs.str();

	}
	else if (mne >> 5 == 0x1f)
	{				//incsz fr
		outs << "incsz " << hex << (mne & 0x1f);
		op = outs.str();

	}
	else if (mne >> 5 == 0x1b)
	{				//RL fr

		outs << "RL " << hex << (mne & 0x1f);
		op = outs.str();

	}
	else if (mne >> 5 == 0x19)
	{				//RR fr
		outs << "RR " << hex << (mne & 0x1f);
		op = outs.str();

	}
	else if (mne >> 5 == 5)
	{				//sub fr,w
		outs << "sub " << hex << (mne & 0x1f) << " w";
		op = outs.str();

	}
	else if (mne >> 5 == 0x1d)
	{				//swap fr
		outs << "swap " << hex << (mne & 0x1f);
		op = outs.str();

	}
	else if (mne >> 8 == 4)
	{				//clrb fr.bit
		int
		bit = (mne >> 5) & 7;
		outs << "clrb " << hex << (mne & 0x1f) << "." << bit;
		op = outs.str();

	}
	else if (mne >> 5 == 0x10)
	{				//mov w,fr
		outs << "mov w, " << hex << (mne & 0x1f);
		op = outs.str();

	}
	else if (mne >> 5 == 0x4)
	{				//mov w,fr-w
		outs << "mov w, " << hex << (mne & 0x1f) << "-w";
		op = outs.str();

	}
	else if (mne >> 5 == 0x12)
	{				//mov w,/fr (complement)
		outs << "mov w, /" << hex << (mne & 0x1f);
		op = outs.str();

	}
	else if (mne >> 5 == 0x6)
	{				//mov w,--fr
		outs << "mov w, --" << hex << (mne & 0x1f);
		op = outs.str();

	}
	else if (mne >> 5 == 0x14)
	{				//mov w,++fr
		outs << "mov w, ++" << hex << (mne & 0x1f);
		op = outs.str();

	}
	else if (mne >> 5 == 0x1a)
	{				//mov w,<<fr
		outs << "mov w, <<" << hex << (mne & 0x1f);
		op = outs.str();

	}
	else if (mne >> 5 == 0x18)
	{				//mov w,>>fr
		outs << "mov w, <<" << hex << (mne & 0x1f);
		op = outs.str();

	}
	else if (mne == 0x43)
	{				//TODO: mov m,w
		outs << "mov m, w" ;
		op = outs.str();

	}
	else if (mne == 0x42)
	{				//TODO: mov w,m
		outs << "mov w, m" << hex << (mne & 0x1f);
		op = outs.str();

	}

	else if (mne >> 5 == 0x16)
	{				//movsz w,--fr
		outs << "movsz w, --" << hex << (mne & 0x1f);
		op = outs.str();

	}
	else if (mne >> 5 == 0x1e)
	{				//movsz w,++fr
		outs << "movsz w, ++" << hex << (mne & 0x1f);
		op = outs.str();

	}
	else if (mne == 0x43)
	{ //mov m,w
		outs << "mov m, w";
		op = outs.str();

	}
	else if (mne >> 4 == 0x5)
	{ //mov m,#lit
		outs << "mov m, #" << hex << (mne&0xf);
		op = outs.str();

	}
	else if (mne == 0x2)
	{ //mov !option,w
		outs << "mov !option, w";
		op = outs.str();

	}
	else if (mne > 4 && mne < 10)
	{
		outs << "mov !rx, w";
		op = outs.str();

	}
	else if (mne == 2)
	{ //mov !option,w
		outs << "not impl: mov !option, w";
		op = outs.str();

	}

	else if (mne >> 5 == 0x11)
	{				//test fr
		outs << "test " << hex << (mne & 0x1f);
		op = outs.str();

	}
	else if (mne == 0)
	{				//nop
		outs << "nop";
		op = outs.str();

	}
	else if (mne >> 8 == 9)
	{ //call addr8
		outs << "call " << hex << (mne & 0xff); // the page comes from STATUS, unknown here
		op = outs.str();

	}
	else if (mne == 0xc)
	{ //ret
		outs << "ret";
		op = outs.str();
	}
	else if (mne == 0xd)
	{ //retp
		outs << "retp";
		op = outs.str();

	}
	else if (mne >> 3 == 3)
	{ //bank addr12
		outs << "bank " << hex << (mne&7);
		op = outs.str();

	}
	else if (mne >> 3 == 2)
	{ //page addr12
		outs << "page " << hex << (mne&7);
		op = outs.str();

	}
	else if (mne == 0x41)
	{ //IREAD
		outs << "IREAD";
		op = outs.str();
	}
	else if (mne == 0xe)
	{ //TODO: reti
		outs << "not impl: reti";
		op = outs.str();
	}
	else if (mne == 0xf)
	{ //TODO: retiw
		outs << "not impl: retiw";
		op = outs.str();
	}
	else if (mne >> 8 == 8)
	{ //retw #lit
		outs << "retw " << hex << (mne&0xff);
		op = outs.str();
	}
	else if (mne == 3)
	{ //TODO: SLEEP
		outs << "SLEEP";
		op = outs.str();
	}



	else
	{
		outs << "not impl " << hex << mne;
		op = outs.str();
	}
	return op;

}
