#ifndef XGSBOARD_H_
#define XGSBOARD_H_

#include "board_ports.h"
#include "cpu_ports.h"

/**
 * What the XGameStation Micro Edition has around the SX52: the video on port RE, the sound chip
 * on port RA, the serial joystick interface on port RA and the 128 KB SRAM on the ports RA, RC and
 * RD. The CPU talks to it through PortDevice.
 *
 * Pins of RA: bit 0 is the clock and bit 1 the "start" of the joystick, bit 2 its data
 * (low: pressed), bit 3..6 the sound chip (see Bu8763), bit 7 the highest address bit of the SRAM.
 * Pins of RC: bits 0..3 are the lowest address bits of the SRAM, bit 4 is the clock of the 12 bit
 * page register (it takes bit 5 with the rising edge), bit 6 is the chip enable (low: enabled)
 * and bit 7 the direction (low: write). RD is its data bus.
 */
class XgsBoard : public PortDevice
{
public:
	static const int SRAM_SIZE = 131072;

	/**
	 * @param video receives the colour of port RE, or NULL if there is no screen
	 * @param sound receives the value of port RA, or NULL if there is no sound chip
	 */
	XgsBoard(VideoPort* video, SoundPort* sound);

	/** Clears the SRAM and the state of the joystick interface and the address logic. */
	void reset();

	/**
	 * The buttons of the first joystick: 1 up, 2 down, 4 left, 8 right, 0x10 fire. The program
	 * reads them as a 16 bit word, most significant bit first, one bit per clock pulse.
	 */
	void setJoystick(unsigned short buttons);
	unsigned short getJoystick() const;

	/** The external SRAM (128 KB). It is used in tests. */
	unsigned char* getSRAM();

	virtual void portWritten(int port, unsigned char* registers, unsigned int cycle);
	virtual void portChanged(int port, unsigned char* registers);

private:
	XgsBoard(const XgsBoard&) = delete;
	XgsBoard& operator=(const XgsBoard&) = delete;

	void joystick(unsigned char* registers);
	void sram(unsigned char* registers);
	void sramStore(unsigned char* registers);
	int sramAddress(const unsigned char* registers) const;

	VideoPort* video;
	SoundPort* sound;
	unsigned char SRAM[SRAM_SIZE];
	unsigned short buttons;
	bool joystickClockHigh;
	char joystickBit;
	unsigned short sramPage;
	bool sramClockHigh;
};

#endif /*XGSBOARD_H_*/
