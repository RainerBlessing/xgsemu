#include "Video.h"
#include <stdio.h>
#include <math.h>

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


void Video::changeVideo(int cycle, unsigned char colour)
{
	static int frame = 0;
	static bool vsync, line;
	static int fragmentcounter = 0;
	static int y, x, yc;
	unsigned char *ubuff8;
	static Uint32 burst;
	Uint32 *ubuff32;

	slfragments[fragmentcounter].cycles = cycle;
	if ((slfragments[fragmentcounter].colour &0xf) == 0)
	{

		if ((fragmentcounter > 0 && cycle - slfragments[fragmentcounter - 1].cycles > 4000) ||
		        (fragmentcounter == 0 && cycle - slfragments[999].cycles > 4000))
		{
			fragmentcounter = yc = 0;
			y = 0;
			vsync = true;
			if (frame == 100)frame = 0;else
			frame++;
			//SDL_BlitSurface(bscreen, NULL, screen, NULL);
			//SDL_UpdateRect(screen, 0, 0, 0, 0);
		}

		else if ((fragmentcounter > 0 && cycle - slfragments[fragmentcounter - 1].cycles > 360) || //376
		         (fragmentcounter == 0 && cycle - slfragments[999].cycles > 360))
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
		if (line == false && cycle - slfragments[fragmentcounter - 1].cycles > 40 && cycle - slfragments[fragmentcounter - 1].cycles < 50)
		{
			line = true;
			burst = colour >> 4;
		}
		else if ( vsync == true && line == true && frame % 2 == 0)
		{

			static int x0 = 0;

			if (fragmentcounter > 0)
			{
				x0 = (slfragments[fragmentcounter - 1].cycles - slfragments[yc].cycles) >> 3;
				x = ((cycle - slfragments[yc].cycles) >> 3);
			}
			else if ( yc != 0)
			{
				x0 = (slfragments[999].cycles - slfragments[yc].cycles) >> 3;
				x = ((cycle - slfragments[yc].cycles) >> 3);
			}

			if ( SDL_MUSTLOCK(screen) )
			{
				if ( SDL_LockSurface(screen) < 0 )
				{
					return ;
				}
			}

			if (x0 < 50)x0 = 50;
			if (x < 50)x = 50;
			if (x > 560)x = 560;
			if (x0 > 560)x0 = 560;

			x0 = (x0 - 50) * 1.1;
			x = (x - 50) * 1.1;
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

	fragmentcounter++;

	if (fragmentcounter == 1000)fragmentcounter = 0;
	slfragments[fragmentcounter].colour = colour;

}

