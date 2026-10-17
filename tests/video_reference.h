// Frozen copy of Video from before the refactoring of Video::changeVideo. It is the
// reference for the differential test and is removed when the refactoring is done.
#ifndef VIDEO_REFERENCE_H_
#define VIDEO_REFERENCE_H_
#include "SDL.h"
#define USHORT unsigned short

typedef struct
{
	int cycles;
	USHORT colour;
}
FragmentReference;
/**
 * The class responsible for displaying something.
 */
class VideoReference
{
public:
	VideoReference();
	virtual ~VideoReference();
	/**
	 * This function is called if something is written to the RE
	 * register i.e. the colour changes.<br>
	 * The cycle and colour are saved in an array which is circularly read 
	 * The length of a line fragment is calculated by subtracting the 
	        * last cycle count.<br>
	 * The video timing depends on the sx52 clock. 
	 * @param cycle The current clock cycle
	 * @param colour The colour of the line fragment
	 */
	virtual void changeVideo(int cycle, unsigned char colour);
	/**
	 * Forgets the timing of the old signal. Call it when the cycle counter
	 * starts again at 0, i.e. when a new program is loaded or the CPU is reset.
	 */
	void reset();
	/**
	 * Returns the pointer to the SDL surface
	 * @return Screen pointer
	 */
	SDL_Surface* getScreen()
	{
		return screen;
	};
private:
	/**
	 * Number of line segments in the ring buffer
	 */
	static const int FRAGMENTS = 1000;
	/**
	 * The array with the line segments
	 */
	FragmentReference slfragments[FRAGMENTS];
	/**
	 * Where the signal currently is. The fragments of one video line are
	 * collected in the ring buffer and drawn when their end is known.
	 */
	int fragmentcounter; // current index in the ring buffer
	int yc;              // index of the fragment where the current line started
	int frame;           // counts the vsyncs, only every second frame is drawn
	bool vsync;          // a vsync has been seen
	bool line;           // the colour burst of the current line has been seen
	int x0, x, y;        // pixel position of the fragment that is drawn
	Uint32 burst;        // colour of the colour burst, reference for the phase
	/**
	 * 28 (2*14) different colours
	 */
	Uint32 colours[28];
	/**
	        * Intensity levels
	        */
	Uint32 intensity[16];
	SDL_Surface* screen;
};

#endif /*VIDEO_REFERENCE_H_*/
