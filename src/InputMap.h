#ifndef INPUTMAP_H_
#define INPUTMAP_H_

#include <SDL_keycode.h>

/**
 * The joystick button a key stands for: 1 up, 2 down, 4 left, 8 right, 0x10 fire. 0 for every
 * other key.
 */
inline unsigned short joystickMaskForKey(SDL_Keycode key)
{
	switch (key)
	{
		case SDLK_UP:
			return 0x01;
		case SDLK_DOWN:
			return 0x02;
		case SDLK_LEFT:
			return 0x04;
		case SDLK_RIGHT:
			return 0x08;
		case SDLK_LCTRL:
		case SDLK_RCTRL:
		case SDLK_SPACE:
			return 0x10;
		default:
			return 0;
	}
}

#endif /*INPUTMAP_H_*/
