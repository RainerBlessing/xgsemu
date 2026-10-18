#ifndef MACHINE_H_
#define MACHINE_H_

#include <memory>
#include <string>

class Cpu;
class Video;
class Bu8763;

/**
 * The XGameStation Micro Edition without a window: the SX52 with the program, the video
 * it generates, the sound chip and the joystick. A front end loads a program, calls
 * runFrame() 60 times a second, shows getVideo() and plays getSound(). It does not need SDL.
 *
 * Everything but getSound().render() is for one thread; the sound chip is the one thing the
 * audio thread reads while the emulator runs.
 */
class Machine
{
public:
	/** Clock cycles of one frame: 80 MHz / 60 fps. */
	static const unsigned int CYCLES_PER_FRAME = 1333333;

	Machine();
	~Machine();

	/**
	 * Loads a .hex or .obj program and starts it from the beginning. If the file cannot be
	 * read the running program (and the picture) stay as they are.
	 * @return false if the file could not be read
	 */
	bool load(const std::string& path);

	/** Is there a program? */
	bool isLoaded() const;

	/** Starts the program again from the beginning. Does nothing without a program. */
	void reset();

	/** Runs the program for one video frame. Does nothing without a program. */
	void runFrame();

	/**
	 * Presses or releases joystick buttons: 1 up, 2 down, 4 left, 8 right, 0x10 fire.
	 * The program reads the joystick once per frame. A button that is released before a frame
	 * has run with it pressed stays pressed until that frame is over, so that a short tap is
	 * not lost. A button that is held stays pressed when another program is loaded.
	 */
	void setJoystick(unsigned short mask, bool down);

	/** The buttons the program sees as pressed. */
	unsigned short getJoystick() const;

	/** The picture. It is there without a program as well, and black. */
	const Video& getVideo() const;

	/** The sound chip. It lives as long as the machine, a loaded program only resets it. */
	Bu8763& getSound();

private:
	Machine(const Machine&) = delete;
	Machine& operator=(const Machine&) = delete;

	void passJoystickToCpu();

	std::unique_ptr<Video> video;
	std::unique_ptr<Bu8763> sound;
	std::unique_ptr<Cpu> cpu;
	unsigned short joystick;
	unsigned short pressedUnseen;	// pressed, but no frame has run with it yet
	unsigned short releaseDeferred;	// released while still unseen: waits for the end of the frame
};

#endif /*MACHINE_H_*/
