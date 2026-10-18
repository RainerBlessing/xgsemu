#ifndef VIDEO_H_
#define VIDEO_H_
#include <stdint.h>
#include <vector>
#include "board_ports.h"
#define USHORT unsigned short

typedef struct
{
	int cycles;
	USHORT colour;
}
Fragment;
/**
 * The class responsible for displaying something.
 */
class Video : public VideoPort
{
public:
	Video();
	virtual ~Video();
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
	 * Size of the picture in pixels.
	 */
	static const int WIDTH = 560;
	static const int HEIGHT = 525;
	int width() const
	{
		return WIDTH;
	};
	int height() const
	{
		return HEIGHT;
	};
	/**
	 * The picture: HEIGHT rows of WIDTH pixels, row after row, no padding. A pixel is
	 * 0xAARRGGBB with the alpha of a drawn pixel at 0xFF; what was never drawn is 0.
	 */
	uint32_t* pixels()
	{
		return &picture[0];
	};
	const uint32_t* pixels() const
	{
		return &picture[0];
	};
	/** Bytes from the start of one row to the start of the next. */
	int pitch() const
	{
		return WIDTH * (int) sizeof(uint32_t);
	};
private:
	/**
	 * Number of line segments in the ring buffer
	 */
	static const int FRAGMENTS = 1000;
	/**
	 * The array with the line segments
	 */
	Fragment slfragments[FRAGMENTS];
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
	uint32_t burst;      // colour of the colour burst, reference for the phase
	/**
	        * Intensity levels
	        */
	uint32_t intensity[16];
	std::vector<uint32_t> picture;

	void startFrame();
	void startLine();
	void measureFragment(int cycle, int previous);
	int toScreenColumn(int position) const;
	static void wheelColour(int phase, int* red, int* green, int* blue);
	uint32_t fragmentPixel(unsigned short colour) const;
	void fillRow(int row, int from, int to, uint32_t pixel);
	void drawFragment(int cycle, int previous);
};

#endif /*VIDEO_H_*/
