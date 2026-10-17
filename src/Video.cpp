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

// The colour wheel: the phase of a colour against the colour burst. The hardware
// delays the colour signal in steps of about 12 degrees, so the wheel has 30 steps.
// With the burst at chroma 0 the chroma 0 is yellow (60 degrees of hue), 5 is red,
// 10 is magenta and 14 blue-violet (colour chart of Rem-Pac, figure 11.33 of the book).
const int WHEEL_STEPS = 30;
const int STEP_DEGREES = 12;
const int YELLOW_DEGREES = 60;

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

// The colour of the wheel (red, green and blue as 0..255) for the phase of a
// colour against the colour burst, in steps of the wheel. The colour is fully
// saturated, the hue goes down with the phase.
void Video::wheelColour(int phase, int* red, int* green, int* blue)
{
	int hue = (YELLOW_DEGREES - STEP_DEGREES * phase) % 360;
	if (hue < 0) hue += 360;
	int rising = 255 * (hue % 60) / 60;
	int falling = 255 - rising;
	switch (hue / 60)
	{
		case 0:  *red = 255;     *green = rising;  *blue = 0;       break;
		case 1:  *red = falling; *green = 255;     *blue = 0;       break;
		case 2:  *red = 0;       *green = 255;     *blue = rising;  break;
		case 3:  *red = 0;       *green = falling; *blue = 255;     break;
		case 4:  *red = rising;  *green = 0;       *blue = 255;     break;
		default: *red = 255;     *green = 0;       *blue = falling; break;
	}
}

// The colour of a fragment: the chroma is the phase against the colour burst, the
// luminance the intensity. Chroma 15 is grey.
Uint32 Video::fragmentPixel(unsigned short colour) const
{
	Uint32 r, g, b;
	Uint32 i = intensity[colour & 0xf];
	int chroma = (colour >> 4) & 0xf;
	if (chroma != 15)
	{
		int phase = (chroma - (int)burst) % WHEEL_STEPS;
		if (phase < 0) phase += WHEEL_STEPS;
		int red, green, blue;
		wheelColour(phase, &red, &green, &blue);
		r = (Uint32)(red * i) >> 8;
		g = (Uint32)(green * i) >> 8;
		b = (Uint32)(blue * i) >> 8;
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
