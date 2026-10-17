/*
* =====================================================================================
* 
*        Filename:  cpu.cpp
* 
*     Description:  
* 
*         Version:  1.0
*         Created:  13.07.2005 23:17:59 CEST
*        Revision:  none
*        Compiler:  gcc
* 
*          Author:  Rainer Blessing (), rainer@theblessing.net
*         Company:  
* 
* =====================================================================================
*/
#include <string.h>
#include <sstream>
#include <string>
#include "SDL.h"
#include "Video.h"
//#include "Windows.h"

using namespace std;
#include "cpu.h"

Cpu::Cpu (Video* video)
{
	rtcc = &RAM[1];
	status = &RAM[3];
	fsr = &RAM[4];
	ra = &RAM[5];
	rb = &RAM[6];
	rc = &RAM[7];
	rd = &RAM[8];
	re = &RAM[9];
	w = &RAM[261];
	t1 = SDL_GetTicks();
	reset();
	this->video = video;
	sound = NULL;
	joystickr = 0;
}

void Cpu::setSound(Bu8763* sound)
{
	this->sound = sound;
	lastRa = *ra;
}

void Cpu::reset()
{
	memset (RAM, 0, 262);
	memset (Tris, 0, sizeof(Tris));
	memset (Stack, 0, sizeof(Stack));
	sramPage = 0;
	sramActive = false;
	joystickClockHigh = false;
	joystickBit = 0;
	mode = 0;
	option = 0;
	memset (SRAM, 0, 131072*sizeof(unsigned char));
	cycle = 0;
	cycle2 = 0;
	overshoot = 0;
	prescaler = 0;
	lastRa = 0;
	sp = Stack;
	pc = 0xfff;
}

USHORT *
Cpu::getROM ()
{
	return ROM;
}

void Cpu::runCycles(unsigned int count)
{
	unsigned int target = cycle + count - overshoot;
	// signed difference, the counter may run over
	while ((int)(cycle - target) < 0)
	{
		execute();
	}
	overshoot = cycle - target;
}

unsigned char *
Cpu::getRAM ()
{
	return RAM;
}

unsigned char *
Cpu::getSRAM ()
{
	return SRAM;
}

USHORT *
Cpu::getPC ()
{
	return &pc;
}

void Cpu::execute ()
{
	USHORT mne = ROM[pc];

	updateSound();
	updateTimer();
	cycle2 = cycle;

	(this->*lookupInstruction(mne))(mne);
}

// The sound chip hangs on port RA. Check once per instruction instead of in
// every opcode that can write to the port.
void Cpu::updateSound()
{
	if (*ra != lastRa)
	{
		lastRa = *ra;
		if (sound != NULL)
		{
			sound->portWrite(lastRa);
		}
	}
}

// The timer register (RTCC) counts every 2 << PS cycles.
void Cpu::updateTimer()
{
	prescaler += (int)(cycle - cycle2);
	if (prescaler > (2 << (option&0x7)))
	{
		RAM[1]++;
		prescaler = 0;
	}
}

Cpu::Instruction Cpu::lookupInstruction(USHORT mne)
{
	// by the upper four bits of the 12 bit instruction
	static const Instruction byOpcode[16] = {
		&Cpu::registerInstruction, &Cpu::registerInstruction, &Cpu::registerInstruction, &Cpu::registerInstruction,
		&Cpu::clrb, &Cpu::setb, &Cpu::snb, &Cpu::sb,
		&Cpu::retw, &Cpu::call, &Cpu::jmp, &Cpu::jmp,
		&Cpu::movWLiteral, &Cpu::orWLiteral, &Cpu::andWLiteral, &Cpu::xorWLiteral
	};
	USHORT opcode = (mne >> 8) & 0xFF;
	return opcode < 16 ? byOpcode[opcode] : &Cpu::registerInstruction;
}

// The register of an instruction: 0 is the indirect register (INDF, addressed
// by FSR), 0x10 and above are in the bank selected by FSR.
USHORT Cpu::registerAddress(USHORT mne) const
{
	USHORT fr = mne & 0x1f;
	if (fr == 0)
		return *fsr;
	if (fr > 0xf)
		return (*fsr & 0xf0) | (fr & 0xf);
	return fr;
}

void Cpu::setZero(bool zero)
{
	if (zero)
		*status |= 1 << 2;
	else
		*status &= ~(1 << 2);
}

void Cpu::setCarry(bool carry)
{
	if (carry)
		*status |= 1;
	else
		*status &= ~1;
}

void Cpu::setDigitCarry(bool digitCarry)
{
	if (digitCarry)
		*status |= 1 << 1;
	else
		*status &= ~(1 << 1);
}

// Ends an instruction that continues with the next one.
void Cpu::next(int cycles)
{
	cycle += cycles;
	pc++;
}

// The instruction after a skipping one is not executed, page and bank
// instructions in front of it are skipped as well.
void Cpu::skipFollowing()
{
	pc++;
	cycle++;
	while (ROM[pc] >> 3 == 2 || ROM[pc] >> 3 == 3)
	{
		pc++;
		cycle++;
	}
}

void Cpu::pushReturnAddress(USHORT address)
{
	*sp = address;
	if (sp == Stack + 8)
	{
		sp = Stack;
	}
	else sp++;
}

USHORT Cpu::popReturnAddress()
{
	if (sp == Stack)
	{
		sp = Stack + 8;
	}
	else sp--;
	return *sp;
}

// Writing to a port register has an effect outside of the CPU: RA is the
// joystick and the sound chip, RE is the video.
void Cpu::portWritten(USHORT fr, unsigned char value)
{
	switch (fr - 5)
	{
		case 0:
			joystick();
			break;
		case 4:
			video->changeVideo(cycle, value);
			break;
		default:
			break;
	}
}

// setb/clrb on a port register: the register is not changed like RAM, the
// bit goes out to the pin. Returns false for all other registers.
bool Cpu::portBitWritten(USHORT fr, USHORT bit, bool set)
{
	if (!(fr > 0x4 && fr < 0xa && (~Tris[fr - 5]&bit) == bit))
	{
		return false;
	}
	switch (fr - 5)
	{
		case 0:
			if (set)
				*ra |= 1 << bit;
			else
				*ra &= ~(1 << bit);
			joystick();
			break;
		case 2:
			if (set)
				*rc |= 1 << bit;
			else
				*rc &= ~(1 << bit);
			sram();
			break;
		default:
			break;
	}
	next();
	return true;
}

void Cpu::jmp(USHORT mne)
{
	pc = ((*status & 0xe0) << 4) | (mne & 0x1ff);
	cycle += 3;
}

void Cpu::call(USHORT mne)
{
	pushReturnAddress(pc + 1);
	pc = (*status & 0xe0) << 4 | mne & 0xff;
	cycle += 3;
}

void Cpu::retw(USHORT mne)
{
	pc = popReturnAddress();
	*w = mne & 0xff;
	cycle += 3;
}

void Cpu::movWLiteral(USHORT mne)
{
	*w = mne & 0xff;
	next();
}

void Cpu::andWLiteral(USHORT mne)
{
	USHORT literal = mne & 0xff;
	if (literal == 0)
	{
		literal = *fsr;
	}
	*w = *w & literal;
	setZero(*w == 0);
	next();
}

void Cpu::orWLiteral(USHORT mne)
{
	*w = *w | (mne & 0xff);
	setZero(*w == 0);
	next();
}

void Cpu::xorWLiteral(USHORT mne)
{
	*w ^= mne & 0xff;
	setZero(*w == 0);
	next();
}

void Cpu::setb(USHORT mne)
{
	USHORT bit = (mne >> 5) & 7;
	if (portBitWritten(mne & 0x1f, bit, true))
		return;
	RAM[registerAddress(mne)] |= 1 << bit;
	next();
}

void Cpu::clrb(USHORT mne)
{
	USHORT bit = (mne >> 5) & 7;
	if (portBitWritten(mne & 0x1f, bit, false))
		return;
	RAM[registerAddress(mne)] &= (~(1 << bit));
	next();
}

void Cpu::snb(USHORT mne)
{
	USHORT bit = (mne >> 5) & 7;
	if (!(RAM[registerAddress(mne)] >> bit & 1))
	{
		skipFollowing();
	}
	next();
}

void Cpu::sb(USHORT mne)
{
	USHORT bit = (mne >> 5) & 7;
	if (((RAM[registerAddress(mne)] >> bit) & 1) == 1)
	{
		skipFollowing();
	}
	next();
}

// Instructions with a register operand, selected by the upper seven bits of
// the 12 bit instruction (the lowest of them is the destination bit). What
// is not in the table are the special instructions.
void Cpu::registerInstruction(USHORT mne)
{
	static const Instruction bySevenBits[32] = {
		&Cpu::controlInstruction, &Cpu::movFrW, &Cpu::controlInstruction, &Cpu::clrFr,
		&Cpu::movWFrMinusW, &Cpu::subFrW, &Cpu::movWDecFr, &Cpu::decFr,
		&Cpu::orWFr, &Cpu::orFrW, &Cpu::andWFr, &Cpu::andFrW,
		&Cpu::xorWFr, &Cpu::xorFrW, &Cpu::addWFr, &Cpu::addFrW,
		&Cpu::movWFr, &Cpu::testFr, &Cpu::movWNotFr, &Cpu::notFr,
		&Cpu::movWIncFr, &Cpu::incFr, &Cpu::movszDec, &Cpu::decsz,
		&Cpu::movWRrFr, &Cpu::rrFr, &Cpu::movWRlFr, &Cpu::rlFr,
		&Cpu::movWSwapFr, &Cpu::swapFr, &Cpu::movszInc, &Cpu::incsz
	};
	USHORT sevenBits = mne >> 5;
	(this->*(sevenBits < 32 ? bySevenBits[sevenBits] : &Cpu::controlInstruction))(mne);
}

// w = value, the zero flag follows
void Cpu::setW(unsigned char value)
{
	*w = value;
	setZero(value == 0);
	next();
}

// fr = value, the zero flag follows
void Cpu::setRegister(USHORT fr, unsigned char value)
{
	RAM[fr] = value;
	setZero(value == 0);
	next();
}

void Cpu::movFrW(USHORT mne)
{
	USHORT fr = registerAddress(mne);
	RAM[fr] = *w;
	next();
	portWritten(fr, *w);
}

void Cpu::clrFr(USHORT mne)
{
	USHORT fr = registerAddress(mne);
	RAM[fr] = 0;
	portWritten(fr, 0);
	setZero(true);
	next();
}

void Cpu::decFr(USHORT mne)
{
	USHORT fr = registerAddress(mne);
	RAM[fr] -= 1;
	setZero(RAM[fr] == 0);
	portWritten(fr, RAM[fr]);
	next();
}

void Cpu::incFr(USHORT mne)
{
	USHORT fr = registerAddress(mne);
	RAM[fr]++;
	setZero(RAM[fr] == 0);
	portWritten(fr, RAM[fr]);
	next();
}

void Cpu::decsz(USHORT mne)
{
	USHORT fr = registerAddress(mne);
	RAM[fr] -= 1;
	skipIfZero(RAM[fr]);
}

void Cpu::incsz(USHORT mne)
{
	USHORT fr = registerAddress(mne);
	RAM[fr]++;
	skipIfZero(RAM[fr]);
}

void Cpu::movszDec(USHORT mne)
{
	*w = RAM[registerAddress(mne)] - 1;
	skipIfZero(*w);
}

void Cpu::movszInc(USHORT mne)
{
	*w = RAM[registerAddress(mne)] + 1;
	skipIfZero(*w);
}

// The zero flag follows the value, if it is zero the next instruction is skipped.
void Cpu::skipIfZero(unsigned char value)
{
	setZero(value == 0);
	if (value == 0)
	{
		skipFollowing();
	}
	next();
}

void Cpu::andFrW(USHORT mne)
{
	USHORT fr = registerAddress(mne);
	setRegister(fr, RAM[fr] & *w);
}

void Cpu::orFrW(USHORT mne)
{
	USHORT fr = registerAddress(mne);
	setRegister(fr, RAM[fr] | *w);
}

void Cpu::xorFrW(USHORT mne)
{
	USHORT fr = registerAddress(mne);
	setRegister(fr, RAM[fr] ^ *w);
}

void Cpu::notFr(USHORT mne)
{
	USHORT fr = registerAddress(mne);
	setRegister(fr, ~RAM[fr]);
}

void Cpu::andWFr(USHORT mne)
{
	setW(*w & RAM[registerAddress(mne)]);
}

void Cpu::xorWFr(USHORT mne)
{
	setW(*w ^ RAM[registerAddress(mne)]);
}

void Cpu::orWFr(USHORT mne)
{
	setW(*w | RAM[registerAddress(mne)]);
}

void Cpu::movWFr(USHORT mne)
{
	setW(RAM[registerAddress(mne)]);
}

void Cpu::movWNotFr(USHORT mne)
{
	setW(~RAM[registerAddress(mne)]);
}

void Cpu::movWDecFr(USHORT mne)
{
	setW(RAM[registerAddress(mne)] - 1);
}

void Cpu::movWIncFr(USHORT mne)
{
	setW(RAM[registerAddress(mne)] + 1);
}

void Cpu::movWSwapFr(USHORT mne)
{
	USHORT fr = registerAddress(mne);
	*w = (RAM[fr] >> 4) | ((RAM[fr] & 0xf) << 4);
	next();
}

void Cpu::swapFr(USHORT mne)
{
	USHORT fr = registerAddress(mne);
	RAM[fr] = (RAM[fr] >> 4) | ((RAM[fr] & 0xf) << 4);
	next();
}

void Cpu::testFr(USHORT mne)
{
	setZero(RAM[registerAddress(mne)] == 0);
	next();
}

void Cpu::addFrW(USHORT mne)
{
	USHORT fr = registerAddress(mne);
	setDigitCarry(RAM[fr] < 0x10 && (*w) + RAM[fr] > 0xf);
	USHORT sum;
	if (fr != 2)
		sum = RAM[fr] + *w;
	else
		sum = pc + *w;
	// jmp PC+W: only the low byte of the program counter takes part in the
	// addition, so the carry is normally cleared.
	setCarry((fr != 2 ? sum : ((pc + 1) & 0xff) + *w) > 0xff);

	if (fr != 2)
		RAM[fr] = sum & 0xff;
	else
		pc = sum;

	// the zero flag follows the 8 bit result, not the sum with its carry
	setZero((fr != 2 ? (sum & 0xff) : sum) == 0);
	next((fr == 2) ? 3 : 1);	// jmp PC+W takes 3 cycles
}

void Cpu::addWFr(USHORT mne)
{
	USHORT fr = registerAddress(mne);
	setDigitCarry(*w < 0x10 && (*w) + RAM[fr] > 0xf);
	USHORT sum = *w + RAM[fr];
	setCarry(sum > 0xff);
	setW(sum & 0xff);
}

// Carry and digit carry of a subtraction are set when there is no borrow, from
// bit 7 and from bit 3.
void Cpu::setSubtractionFlags(unsigned char minuend, unsigned char subtrahend)
{
	setCarry(minuend >= subtrahend);
	setDigitCarry((minuend & 0xf) >= (subtrahend & 0xf));
}

void Cpu::subFrW(USHORT mne)
{
	USHORT fr = registerAddress(mne);
	setSubtractionFlags(RAM[fr], *w);
	setRegister(fr, RAM[fr] - *w);
}

void Cpu::movWFrMinusW(USHORT mne)
{
	USHORT fr = registerAddress(mne);
	setSubtractionFlags(RAM[fr], *w);
	setW(RAM[fr] - *w);
}

void Cpu::rlFr(USHORT mne)
{
	USHORT fr = registerAddress(mne);
	bool carryOut = RAM[fr] >> 7 == 1;
	RAM[fr] = (RAM[fr] << 1) | (*status & 1);
	setCarry(carryOut);
	next();
}

void Cpu::rrFr(USHORT mne)
{
	USHORT fr = registerAddress(mne);
	bool carryOut = (RAM[fr] & 1) == 1;
	RAM[fr] = (RAM[fr] >> 1) | ((*status & 1) << 7);
	setCarry(carryOut);
	next();
}

void Cpu::movWRlFr(USHORT mne)
{
	USHORT fr = registerAddress(mne);
	bool carryOut = RAM[fr] >> 7 == 1;
	*w = (RAM[fr] << 1) | (*status & 1);
	setCarry(carryOut);
	next();
}

void Cpu::movWRrFr(USHORT mne)
{
	USHORT fr = registerAddress(mne);
	bool carryOut = (RAM[fr] & 1) == 1;
	*w = (RAM[fr] >> 1) | ((*status & 1) << 7);
	setCarry(carryOut);
	next();
}

// The instructions without a register operand.
void Cpu::controlInstruction(USHORT mne)
{
	if (mne == 0xc)
	{ //ret 0000 0000 1100
		pc = ((*status & 0xe0) << 4) | popReturnAddress();
		cycle += 3;
		return;
	}
	if (mne == 0xd)
	{ //retp
		USHORT address = popReturnAddress();
		// The page bits 7:5 of the status register are the address bits 11:9.
		RAM[3] = ((address >> 4) & 0xe0) | (RAM[3] & 0x1f);
		pc = address;
		cycle += 3;
		return;
	}

	int cycles = 1;
	if (mne == 0x40)
	{ //clr w 0000 0100 0000
		*w = 0;
		setZero(true);
	}
	else if (mne == 0x42)
	{ //mov w,m 0000 0100 0010
		*w = mode;
	}
	else if (mne == 0x43)
	{ //mov m,w 0000 0100 011
		mode = *w;
	}
	else if (mne >> 4 == 0x5)
	{ //mov m,#lit 0000 0101 kkkk
		mode = mne & 0xf;
	}
	else if (mne == 0x2)
	{ //mov !option,w 0000 0000 0010
		option = *w;
	}
	else if (mne >> 3 == 3)
	{ //bank addr12 0000 0001 1nnn
		*fsr = ((mne & 7) << 4) | (*fsr & 0xf);
	}
	else if (mne >> 3 == 2)
	{ //page addr12 0000 0001 0nnn
		*status = ((mne & 7) << 5) | (*status & 0x1f);
	}
	else if (mne == 0x41)
	{ //IREAD 0000 0100 0001
		USHORT address = ((mode & 0xf) << 8) | *w;
		address = ROM[address] & 0xfff;
		mode = (mode & 0xf0) | (address >> 8);
		*w = address & 0xff;
		cycles = 4;
	}
	else if (mne >= 0x5 && mne <= 0x9)
	{ //mov !rx,w
		if (mode == 0xf)
		{
			Tris[mne - 5] = *w;
		}
	}
	// nop, clr !WDT and what is not implemented (reti, retiw, sleep) take a cycle
	next(cycles);
}

void Cpu::joystick()
{
	bool shift = false;
	if ((*ra&2) == 0)
	{
		joystickBit = 15;
	}
	if ((*ra&1) == 1)
	{
		joystickClockHigh = true;
	}
	if ((*ra&1) == 0 && joystickClockHigh == true)
	{
		shift = true;
		joystickClockHigh = false;
	}
	if (shift == true && joystickBit > -1)
	{
		if (((joystickr >> joystickBit) &1) == 1)
		{
			*ra = (*ra & 0xf8) | (*ra & 0x3);
		}
		else
			*ra = (*ra & 0xf8) | 4 | (*ra & 0x3);
		joystickBit--;
	}
}
void Cpu::sram()
{
	if (((*rc >> 4)&1) == 1)
	{
		sramPage <<= 1;
		sramPage &= 0xfff; // 12 bit
		sramPage |= (*rc >> 5) & 1;
	}
	if (((*rc >> 6)&1) == 0)
	{ // chipenable=0
		// 17 bit address: RA7 is the highest bit, then the 12 bit page, the lowest four bits are in RC
		int address = ((*ra >> 7) & 1) << 16 | sramPage << 4 | *rc & 0xf;

		if (((RAM[7] >> 7)&1) == 0)
		{ //write
			SRAM[address] = *rd;
		}
		else
		{ //read
			*rd = SRAM[address];
		}
		sramActive = true;
	}
	if (((RAM[7] >> 6)&1) == 1 && sramActive == true)
	{ // chipenable=1
		sramPage = 0;
		sramActive = false;
	}
}

