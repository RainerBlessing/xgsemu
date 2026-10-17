#ifndef CPU_PORTS_H_
#define CPU_PORTS_H_

/**
 * What the CPU needs from the outside world. The CPU knows only these
 * interfaces, so it can be used without SDL or any other front end.
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

#endif /*CPU_PORTS_H_*/
