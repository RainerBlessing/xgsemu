#include "XgsBoard.h"
#include <string.h>

XgsBoard::XgsBoard(VideoPort* video, SoundPort* sound)
	: video(video),
	  sound(sound),
	  buttons(0)
{
	reset();
}

void XgsBoard::reset()
{
	memset(SRAM, 0, sizeof(SRAM));
	joystickClockHigh = false;
	joystickBit = 0;
	sramPage = 0;
	sramClockHigh = false;
}

void XgsBoard::setJoystick(unsigned short buttons)
{
	this->buttons = buttons;
}

unsigned short XgsBoard::getJoystick() const
{
	return buttons;
}

unsigned char* XgsBoard::getSRAM()
{
	return SRAM;
}

// The program wrote a port register: RA is the joystick, RC and RD the SRAM, RE the video.
void XgsBoard::portWritten(int port, unsigned char* registers, unsigned int cycle)
{
	switch (port)
	{
		case 0:
			joystick(registers);
			break;
		case 2:
			sram(registers);
			break;
		case 3:
			sramStore(registers);
			break;
		case 4:
			if (video != NULL)
				video->changeVideo((int)cycle, registers[4]);
			break;
		default:
			break;
	}
}

// The sound chip hangs on port RA: it wants every value RA has, whoever put it there. The
// SRAM sees its pins change however they were changed, not only by mov, setb and clrb.
void XgsBoard::portChanged(int port, unsigned char* registers)
{
	switch (port)
	{
		case 0:
			if (sound != NULL)
				sound->portWrite(registers[0]);
			break;
		case 2:
			sram(registers);
			break;
		case 3:
			sramStore(registers);
			break;
		default:
			break;
	}
}

// The joystick is a 16 bit shift register: while RA1 is low it starts with the first bit, every
// falling edge of the clock RA0 puts the next bit, most significant first, on RA2 (low: pressed).
void XgsBoard::joystick(unsigned char* registers)
{
	unsigned char& ra = registers[0];
	bool shift = false;
	if ((ra & 2) == 0)
	{
		joystickBit = 15;
	}
	if ((ra & 1) == 1)
	{
		joystickClockHigh = true;
	}
	if ((ra & 1) == 0 && joystickClockHigh == true)
	{
		shift = true;
		joystickClockHigh = false;
	}
	if (shift == true && joystickBit > -1)
	{
		if (((buttons >> joystickBit) & 1) == 1)
		{
			ra &= 0xfb;
		}
		else
			ra |= 4;
		joystickBit--;
	}
}

// The SRAM (AS7C1024, 128K x 8) with its address logic. Two 74HCT164 shift registers hold the
// page, bits 4..15 of the address: RC4 is their clock, they take RC5 with the rising edge.
// Nothing clears them (the reset pins are tied high), so a page stays when the chip is disabled.
// Bits 0..3 of the address are RC0..RC3 and the highest bit is RA7. The chip is enabled by RC6
// low, RC7 selects write (low) or read (high), RD is the data bus.
void XgsBoard::sram(unsigned char* registers)
{
	unsigned char rc = registers[2];
	bool clock = (rc & 0x10) != 0;
	if (clock && !sramClockHigh)
	{
		sramPage = ((sramPage << 1) | ((rc >> 5) & 1)) & 0xfff; // 12 bit
	}
	sramClockHigh = clock;

	if ((rc & 0x40) == 0)
	{ // chip enabled
		int address = sramAddress(registers);
		if ((rc & 0x80) == 0)
		{ // write
			SRAM[address] = registers[3];
		}
		else
		{ // read
			registers[3] = SRAM[address];
		}
	}
}

// The data pins changed while the chip is enabled for writing: the byte is stored.
void XgsBoard::sramStore(unsigned char* registers)
{
	unsigned char rc = registers[2];
	if ((rc & 0x40) == 0 && (rc & 0x80) == 0)
	{
		SRAM[sramAddress(registers)] = registers[3];
	}
}

// 17 bit address: RA7 is the highest bit, then the 12 bit page, the lowest four bits are in RC
int XgsBoard::sramAddress(const unsigned char* registers) const
{
	return (((registers[0] >> 7) & 1) << 16) | (sramPage << 4) | (registers[2] & 0xf);
}
