#include "Video.h"
#include <stdio.h>
#include <math.h>
#include <string.h>

Video::Video()
{
	Uint32 rmask, gmask, bmask, amask;

	#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	rmask = 0xff000000;
	gmask = 0x00ff0000;
	bmask = 0x0000ff00;
	amask = 0x000000ff;
	#else
	rmask = 0x000000ff;
	gmask = 0x0000ff00;
	bmask = 0x00ff0000;
	amask = 0xff000000;
	#endif
	//screen = SDL_CreateRGBSurface(SDL_SWSURFACE,526, 520, 32, rmask, gmask, bmask, 0);
screen = SDL_CreateRGBSurface(0, 560, 525, 32, 
                           0x00FF0000,   // R
                           0x0000FF00,   // G
                           0x000000FF,   // B
                           0xFF000000);  // A
	
	if ( !screen)
	{
		fprintf(stderr, "Couldn't create a surface: %s\n",
		        SDL_GetError());
	}


	reset();

	intensity[0] = 0;
	intensity[1] = 0;
	intensity[2] = 0;
	intensity[3] = 0;
	intensity[4] = 0;
	intensity[5] = 0;
	intensity[6] = 26;
	intensity[7] = 51;
	intensity[8] = 77;
	intensity[9] = 102;
	intensity[10] = 128;
	intensity[11] = 153;
	intensity[12] = 179;
	intensity[13] = 204;
	intensity[14] = 230;
	intensity[15] = 255;

	colours[0] = SDL_MapRGB( screen->format, 0, 255, 255);
	colours[1] = SDL_MapRGB( screen->format, 0, 191, 255);
	colours[2] = SDL_MapRGB( screen->format, 0, 128, 255);
	colours[3] = SDL_MapRGB( screen->format, 0, 64, 255);
	colours[4] = SDL_MapRGB( screen->format, 0, 0, 255);
	colours[5] = SDL_MapRGB( screen->format, 64, 0, 255);
	colours[6] = SDL_MapRGB( screen->format, 128, 0, 255);
	colours[7] = SDL_MapRGB( screen->format, 191, 0, 255);
	colours[8] = SDL_MapRGB( screen->format, 255, 0, 255);
	colours[9] = SDL_MapRGB( screen->format, 255, 0, 191);
	colours[10] = SDL_MapRGB( screen->format, 255, 0, 128);
	colours[11] = SDL_MapRGB( screen->format, 255, 0, 64);
	colours[12] = SDL_MapRGB( screen->format, 255, 0, 0);
	colours[13] = SDL_MapRGB( screen->format, 255, 128, 0);
	colours[14] = SDL_MapRGB( screen->format, 255, 255, 0);
	colours[15] = SDL_MapRGB( screen->format, 0, 255, 255);
	colours[16] = SDL_MapRGB( screen->format, 0, 255, 219);
	colours[17] = SDL_MapRGB( screen->format, 0, 255, 182);
	colours[18] = SDL_MapRGB( screen->format, 0, 255, 146);
	colours[19] = SDL_MapRGB( screen->format, 0, 255, 109);
	colours[20] = SDL_MapRGB( screen->format, 0, 255, 73);
	colours[21] = SDL_MapRGB( screen->format, 0, 255, 36);
	colours[22] = SDL_MapRGB( screen->format, 0, 255, 0);
	colours[23] = SDL_MapRGB( screen->format, 0, 255, 43);
	colours[24] = SDL_MapRGB( screen->format, 0, 255, 85);
	colours[25] = SDL_MapRGB( screen->format, 0, 255, 128);
	colours[26] = SDL_MapRGB( screen->format, 0, 255, 170);
	colours[27] = SDL_MapRGB( screen->format, 0, 255, 213);
}


Video::~Video()
{
	SDL_FreeSurface(screen);
}


namespace
{
// The cycle counter of the CPU runs over after about 27 seconds. Only the
// difference of two counter values is meaningful, so calculate it modulo 2^32.
int elapsed(int now, int before)
{
	return (int)((unsigned int)now - (unsigned int)before);
}

// The video signal, measured in clock cycles of the SX52.
const int VSYNC_CYCLES = 4000;       // a longer sync pulse starts a new frame
const int HSYNC_CYCLES = 360;        // a longer one starts a new line
const int BURST_MIN_CYCLES = 40;     // the colour burst is shorter than 50 and longer than 40
const int BURST_MAX_CYCLES = 50;

// The visible part of a line in units of 8 cycles, and how it is stretched
// to the width of the picture.
const int FIRST_VISIBLE = 50;
const int LAST_VISIBLE = 560;
const double HORIZONTAL_SCALE = 1.1;

// A picture line is drawn twice, the line counter stops here.
const int LAST_LINE = 519;

// A fragment with luminance 0 is a sync pulse.
bool isSync(unsigned short colour)
{
	return (colour & 0xf) == 0;
}
}

void Video::reset()
{
	memset(slfragments, 0, sizeof(slfragments));
	fragmentcounter = 0;
	yc = 0;
	frame = 0;
	vsync = false;
	line = false;
	x0 = x = y = 0;
	burst = 0;
}

// Called whenever the colour on the video port changes. The new colour is for
// the fragment that starts now, what is processed is the fragment that ends
// now: its colour was passed with the previous call.
void Video::changeVideo(int cycle, unsigned char colour)
{
	// The fragments are a ring buffer, the one before index 0 is the last one.
	const int previous = (fragmentcounter + FRAGMENTS - 1) % FRAGMENTS;
	const int sincePrevious = elapsed(cycle, slfragments[previous].cycles);

	slfragments[fragmentcounter].cycles = cycle;
	if (isSync(slfragments[fragmentcounter].colour))
	{
		if (sincePrevious > VSYNC_CYCLES)
			startFrame();
		else if (sincePrevious > HSYNC_CYCLES)
			startLine();
	}
	else if (line == false && sincePrevious > BURST_MIN_CYCLES && sincePrevious < BURST_MAX_CYCLES)
	{
		line = true;
		burst = colour >> 4;
	}
	else if (vsync == true && line == true && frame == 0)
	{
		drawFragment(cycle, previous);
	}

	fragmentcounter++;

	if (fragmentcounter == FRAGMENTS)fragmentcounter = 0;
	slfragments[fragmentcounter].colour = colour;
}

void Video::startFrame()
{
	fragmentcounter = yc = 0;
	y = 0;
	vsync = true;
	frame ^= 1; // only every second frame is drawn
}

void Video::startLine()
{
	x = 0;
	if (y < LAST_LINE)
	{
		y++;
		y++;
	}
	yc = fragmentcounter;
	line = false;
}

// x0 and x are the horizontal start and end of the fragment that ends now, in
// units of 8 cycles from the start of the line.
void Video::measureFragment(int cycle, int previous)
{
	if (fragmentcounter > 0 || yc != 0)
	{
		x0 = elapsed(slfragments[previous].cycles, slfragments[yc].cycles) >> 3;
		x = elapsed(cycle, slfragments[yc].cycles) >> 3;
	}
}

// The column of the picture for a position in units of 8 cycles.
int Video::toScreenColumn(int position) const
{
	if (position < FIRST_VISIBLE) position = FIRST_VISIBLE;
	if (position > LAST_VISIBLE) position = LAST_VISIBLE;
	int column = (position - FIRST_VISIBLE) * HORIZONTAL_SCALE;
	// The scaling can lead past the last column (510 * 1.1 = 561)
	if (column > screen->w) column = screen->w;
	return column;
}

// The colour of a fragment: the chroma is the phase against the colour burst,
// the luminance the intensity. Chroma 15 is grey.
Uint32 Video::fragmentPixel(unsigned short colour) const
{
	Uint32 r, g, b;
	Uint32 base = colours[(((colour >> 4) & 0xf) + burst) % 28];
	Uint32 i = intensity[colour & 0xf];
	if (colour >> 4 != 15)
	{
		r = (Uint32)(((base >> 16) & 0xff) * i) >> 8;
		g = (Uint32)(((base >> 8) & 0xff) * i) >> 8;
		b = (Uint32)(((base) & 0xff) * i) >> 8;
	}
	else r = g = b = i;
	return SDL_MapRGB(screen->format, r, g, b);
}

void Video::fillRow(int row, int from, int to, Uint32 pixel)
{
	unsigned char *start = (unsigned char*)screen->pixels + row * screen->pitch;
	for (int k = from; k < to; k++)
	{
		*(Uint32*)(start + k * screen->format->BytesPerPixel) = pixel;
	}
}

// Draws the fragment that ends now into the current line. A line is drawn
// twice, the picture has twice the lines of the signal.
void Video::drawFragment(int cycle, int previous)
{
	measureFragment(cycle, previous);

	// A surface that needs locking and cannot be locked is skipped,
	// but the signal bookkeeping must go on.
	if (SDL_MUSTLOCK(screen) && SDL_LockSurface(screen) != 0)
	{
		return;
	}

	x0 = toScreenColumn(x0);
	x = toScreenColumn(x);
	Uint32 pixel = fragmentPixel(slfragments[fragmentcounter].colour);
	fillRow(y, x0, x, pixel);
	fillRow(y + 1, x0, x, pixel);

	if (SDL_MUSTLOCK(screen))
	{
		SDL_UnlockSurface(screen);
	}
}
