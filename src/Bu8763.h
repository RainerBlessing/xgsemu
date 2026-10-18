#ifndef BU8763_H_
#define BU8763_H_
#include <mutex>
#include "board_ports.h"

/**
 * Emulation of the ROHM BU8763 melody generator of the XGS ME.
 *
 * The SX52 talks to the chip over port RA:
 * RA3 = SCLK, RA4 = SDATA, RA5 = SSTB, RA6 = CKSEL (master clock select).
 * A command has 11 bits (3 bit address, 8 bit data), MSB first. SDATA is
 * taken over with the rising edge of SCLK, the last 11 bits are applied with
 * the rising edge of SSTB.
 *
 * portWrite() is called by the emulator thread, render() by the audio thread.
 * Only the melody mode is emulated (no DTMF/tone generator). The wave form (sine or
 * square) is one setting for all three channels, the last mode register written counts.
 */
class Bu8763 : public SoundPort
{
public:
	static const int CHANNELS = 3;

	Bu8763();
	void reset();

	/**
	 * Must be called with the new value of RA every time it changes.
	 */
	void portWrite(unsigned char ra);

	/**
	 * Mixes the next samples into a mono 16 bit buffer.
	 */
	void render(short* out, int samples, int sampleRate);

	unsigned char oper() const;
	unsigned char mode(int channel) const;
	unsigned char freq(int channel) const;
	unsigned char gain() const;

	/**
	 * Frequency of the 6 bit note index. The lowest note (0) is A2 (110 Hz).
	 * With CKSEL high the master clock is halved and all notes drop an octave.
	 */
	static double noteFrequency(int index, bool halfClock);

private:
	void latch(int command);

	mutable std::mutex lock;
	// Emulator thread only: serial interface state.
	int shift;
	bool lastSclk;
	bool lastSstb;
	// Shared between the threads, protected by lock.
	unsigned char operReg;
	unsigned char modeReg[CHANNELS];
	unsigned char freqReg[CHANNELS];
	unsigned char gainReg;
	bool sineWave; // the wave form of all three channels: sine or special square
	bool halfClock;
	double phase[CHANNELS];
	double envelope[CHANNELS];
};

#endif /*BU8763_H_*/
