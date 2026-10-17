#ifndef DISPLAYLAYOUT_H_
#define DISPLAYLAYOUT_H_

struct DisplayRect
{
	int x, y, w, h;
};

/**
 * The largest rectangle with the aspect ratio aspectWidth:aspectHeight that fits into
 * the window, centered. The XGS ME produces a television signal, so the picture is
 * shown in 4:3 and not in the shape of the emulated picture memory.
 */
DisplayRect fitAspect(int windowWidth, int windowHeight, int aspectWidth, int aspectHeight);

#endif /*DISPLAYLAYOUT_H_*/
