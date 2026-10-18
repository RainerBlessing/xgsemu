#include "Video.h"
#include <stdio.h>
#include <math.h>
#include <string.h>

Video::Video()
	: picture(WIDTH * HEIGHT, 0)
{
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
}


namespace
{
// The cycle counter of the CPU runs over after about 27 seconds. Only the
// difference of two counter values is meaningful, so calculate it modulo 2^32.
int elapsed(int now, int before)
{
	return (int)((unsigned int)now - (unsigned int)before);
}

// A pixel of the picture: opaque, red, green and blue as 0..255.
uint32_t pack(int red, int green, int blue)
{
	return 0xff000000u | (uint32_t)(red & 0xff) << 16 | (uint32_t)(green & 0xff) << 8 | (uint32_t)(blue & 0xff);
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
// delays the colour signal in 32 steps of 11.25 degrees. In units of a quarter degree:
const int WHEEL_STEPS = 32;
const int STEP_UNITS = 45;           // 11.25 degrees
const int YELLOW_UNITS = 236;        // 59 degrees: with the burst at chroma 0 the chroma 0 is yellow
const int WHEEL_UNITS = 1440;        // 360 degrees
const int SECTOR_UNITS = 240;        // 60 degrees

// How bright the colours of a luminance are: every channel lies between a lowest and a
// highest level, the same for all hues. Up to luminance 8 these are the levels of the colour
// chart of Rem-Pac (burst 0, chroma 0..14), which the wheel reproduces with an error of less
// than 2 of 255 and which are much brighter than the hue scaled with the luminance.
// Above that the chart goes pastel (the lowest level up to 167), but photos of the real
// hardware stay saturated: the orange sky, the meadow and the kerbs of Racer City have a
// lowest channel of 2 to 33% of the highest one, the capture of the plane demo 24% at the
// highest luminance. So the lowest level is limited to about 30% of the highest one there.
// Below the black level (luminance 0..4) the colours are black.
const int LOWEST[16]  = {0, 0, 0, 0, 0,  14,  21,  37,  58,  66,  68,  70,  72,  74,  76,  78};
const int HIGHEST[16] = {0, 0, 0, 0, 0,  75, 129, 186, 230, 234, 236, 240, 254, 254, 254, 255};

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
	if (column > WIDTH) column = WIDTH;
	return column;
}

// The colour of the wheel (red, green and blue as 0..255) for the phase of a
// colour against the colour burst, in steps of the wheel. The colour is fully
// saturated, the hue goes down with the phase.
void Video::wheelColour(int phase, int* red, int* green, int* blue)
{
	int hue = (YELLOW_UNITS - STEP_UNITS * phase) % WHEEL_UNITS;
	if (hue < 0) hue += WHEEL_UNITS;
	int rising = 255 * (hue % SECTOR_UNITS) / SECTOR_UNITS;
	int falling = 255 - rising;
	switch (hue / SECTOR_UNITS)
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
// luminance gives the range of the channels. Chroma 15 is grey: the luminance itself.
uint32_t Video::fragmentPixel(unsigned short colour) const
{
	int luminance = colour & 0xf;
	int chroma = (colour >> 4) & 0xf;
	if (chroma == 15)
	{
		uint32_t i = intensity[luminance];
		return pack(i, i, i);
	}

	int phase = (chroma - (int)burst) % WHEEL_STEPS;
	if (phase < 0) phase += WHEEL_STEPS;
	int red, green, blue;
	wheelColour(phase, &red, &green, &blue);
	int lowest = LOWEST[luminance];
	int range = HIGHEST[luminance] - lowest;
	return pack(lowest + range * red / 255,
	            lowest + range * green / 255,
	            lowest + range * blue / 255);
}

void Video::fillRow(int row, int from, int to, uint32_t pixel)
{
	uint32_t* start = &picture[0] + row * WIDTH;
	for (int k = from; k < to; k++)
	{
		start[k] = pixel;
	}
}

// Draws the fragment that ends now into the current line. A line is drawn
// twice, the picture has twice the lines of the signal.
void Video::drawFragment(int cycle, int previous)
{
	measureFragment(cycle, previous);

	x0 = toScreenColumn(x0);
	x = toScreenColumn(x);
	uint32_t pixel = fragmentPixel(slfragments[fragmentcounter].colour);
	fillRow(y, x0, x, pixel);
	fillRow(y + 1, x0, x, pixel);
}
