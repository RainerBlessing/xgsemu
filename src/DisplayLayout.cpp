#include "DisplayLayout.h"

DisplayRect fitAspect(int windowWidth, int windowHeight, int aspectWidth, int aspectHeight)
{
	DisplayRect rect = {0, 0, 0, 0};
	if (windowWidth <= 0 || windowHeight <= 0 || aspectWidth <= 0 || aspectHeight <= 0)
	{
		return rect;
	}

	// Compare the shapes without a division: window wider than the aspect ratio?
	if ((long long)windowWidth * aspectHeight > (long long)windowHeight * aspectWidth)
	{
		rect.h = windowHeight;
		rect.w = (int)((long long)windowHeight * aspectWidth / aspectHeight);
	}
	else
	{
		rect.w = windowWidth;
		rect.h = (int)((long long)windowWidth * aspectHeight / aspectWidth);
	}
	rect.x = (windowWidth - rect.w) / 2;
	rect.y = (windowHeight - rect.h) / 2;
	return rect;
}
