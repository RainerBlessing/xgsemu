#ifndef VIDEO_H_
#define VIDEO_H_
#include "SDL.h"
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
class Video
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
	void changeVideo(int cycle, unsigned char colour);
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
	 * The array with the line segments
	 */
	Fragment slfragments[1000];
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

#endif /*VIDEO_H_*/
