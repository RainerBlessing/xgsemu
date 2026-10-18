#ifndef BOARD_PORTS_H_
#define BOARD_PORTS_H_

/**
 * What hangs on the ports of the SX52 on the XGS ME, as the board hands it to them (see
 * XgsBoard). They know nothing about the CPU.
 */

/** Receives the colour that the program writes to port RE. */
class VideoPort
{
public:
	virtual ~VideoPort() {}
	/**
	 * @param cycle The current clock cycle
	 * @param colour The new value of RE
	 */
	virtual void changeVideo(int cycle, unsigned char colour) = 0;
};

/** Receives the value of port RA, where the sound chip hangs. */
class SoundPort
{
public:
	virtual ~SoundPort() {}
	virtual void portWrite(unsigned char ra) = 0;
};

#endif /*BOARD_PORTS_H_*/
