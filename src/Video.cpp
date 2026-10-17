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

void Video::changeVideo(int cycle, unsigned char colour)
{
	unsigned char *ubuff8;
	Uint32 *ubuff32;
	// The fragments are a ring buffer, the one before index 0 is the last one.
	const int previous = (fragmentcounter + FRAGMENTS - 1) % FRAGMENTS;
	const int sincePrevious = elapsed(cycle, slfragments[previous].cycles);

	slfragments[fragmentcounter].cycles = cycle;
	if ((slfragments[fragmentcounter].colour &0xf) == 0)
	{

		if (sincePrevious > 4000)
		{
			fragmentcounter = yc = 0;
			y = 0;
			vsync = true;
			frame ^= 1; // only every second frame is drawn
			//SDL_BlitSurface(bscreen, NULL, screen, NULL);
			//SDL_UpdateRect(screen, 0, 0, 0, 0);
		}

		else if (sincePrevious > 360) //376
		{
			x = 0;
			if(y<519){
				y++;
				y++;
			}
			yc = fragmentcounter;
			line = false;
		}
	}
	else
		if (line == false && sincePrevious > 40 && sincePrevious < 50)
		{
			line = true;
			burst = colour >> 4;
		}
		else if ( vsync == true && line == true && frame == 0)
		{
			if (fragmentcounter > 0)
			{
				x0 = elapsed(slfragments[previous].cycles, slfragments[yc].cycles) >> 3;
				x = elapsed(cycle, slfragments[yc].cycles) >> 3;
			}
			else if ( yc != 0)
			{
				x0 = elapsed(slfragments[previous].cycles, slfragments[yc].cycles) >> 3;
				x = elapsed(cycle, slfragments[yc].cycles) >> 3;
			}

			// A surface that needs locking and cannot be locked is skipped,
			// but the signal bookkeeping below must go on.
			bool locked = true;
			if ( SDL_MUSTLOCK(screen) )
			{
				locked = SDL_LockSurface(screen) == 0;
			}

			if (locked)
			{
				if (x0 < 50)x0 = 50;
				if (x < 50)x = 50;
				if (x > 560)x = 560;
				if (x0 > 560)x0 = 560;

				x0 = (x0 - 50) * 1.1;
				x = (x - 50) * 1.1;
				// The scaling can lead past the last column (510 * 1.1 = 561)
				if (x0 > screen->w)x0 = screen->w;
				if (x > screen->w)x = screen->w;
				//x0=(x0-100)*1.23;
				//x=(x-100)*1.23;

				Uint32 r, g, b, i;
				Uint32 buff32 = colours[(((slfragments[fragmentcounter].colour >> 4) & 0xf) + burst) % 28];
				i = intensity[slfragments[fragmentcounter].colour & 0xf];
				ubuff8 = (unsigned char*)screen->pixels + y * screen->pitch;
				if (slfragments[fragmentcounter].colour >> 4 != 15)
				{
					r = (Uint32)(((buff32 >> 16) & 0xff) * i) >> 8;
					g = (Uint32)(((buff32 >> 8) & 0xff) * i) >> 8;
					b = (Uint32)(((buff32) & 0xff) * i) >> 8;
				}
				else r = g = b = i;
				r = SDL_MapRGB(screen->format, r, g, b);
				int k;
				for (k = x0;k < x;k++)
				{
					ubuff32 = (Uint32*)(ubuff8 + k * screen->format->BytesPerPixel);
					*ubuff32 = r;
				}
				ubuff8 = (unsigned char*)screen->pixels + (y + 1) * screen->pitch;
				for (k = x0;k < x;k++)
				{
					ubuff32 = (Uint32*)(ubuff8 + k * screen->format->BytesPerPixel);
					*ubuff32 = r;
				}

				if ( SDL_MUSTLOCK(screen) )
				{
					SDL_UnlockSurface(screen);
				}
			}
		}

	fragmentcounter++;

	if (fragmentcounter == FRAGMENTS)fragmentcounter = 0;
	slfragments[fragmentcounter].colour = colour;

}
