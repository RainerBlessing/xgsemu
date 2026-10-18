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
	sramActive = false;
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

// The program wrote a port register: RA is the joystick, RC the SRAM, RE the video.
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
		case 4:
			if (video != NULL)
				video->changeVideo((int)cycle, registers[4]);
			break;
		default:
			break;
	}
}

// The sound chip hangs on port RA: it wants every value RA has, whoever put it there.
void XgsBoard::portChanged(int port, unsigned char* registers)
{
	if (port == 0 && sound != NULL)
	{
		sound->portWrite(registers[0]);
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

void XgsBoard::sram(unsigned char* registers)
{
	unsigned char ra = registers[0];
	unsigned char rc = registers[2];
	unsigned char& rd = registers[3];
	if (((rc >> 4) & 1) == 1)
	{
		sramPage <<= 1;
		sramPage &= 0xfff; // 12 bit
		sramPage |= (rc >> 5) & 1;
	}
	if (((rc >> 6) & 1) == 0)
	{ // chipenable=0
		// 17 bit address: RA7 is the highest bit, then the 12 bit page, the lowest four bits are in RC
		int address = (((ra >> 7) & 1) << 16) | (sramPage << 4) | (rc & 0xf);

		if (((rc >> 7) & 1) == 0)
		{ //write
			SRAM[address] = rd;
		}
		else
		{ //read
			rd = SRAM[address];
		}
		sramActive = true;
	}
	if (((rc >> 6) & 1) == 1 && sramActive == true)
	{ // chipenable=1
		sramPage = 0;
		sramActive = false;
	}
}
