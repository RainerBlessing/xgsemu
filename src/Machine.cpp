#include "Machine.h"
#include <string.h>
#include "Bu8763.h"
#include "Video.h"
#include "cpu.h"
#include "filehandler.h"

Machine::Machine()
	: video(new Video()),
	  sound(new Bu8763()),
	  joystick(0),
	  pressedUnseen(0),
	  releaseDeferred(0)
{
}

Machine::~Machine()
{
	// The members go in reverse order: first the CPU that points to the video and the sound chip.
}

bool Machine::load(const std::string& path)
{
	// Read the file first: if it cannot be read the running program stays as it is
	u16 rom[4096];
	u16 fusex;
	if (!FileHandler::readFile(path.c_str(), rom, &fusex))
	{
		return false;
	}

	// a new video: the program counts cycles from 0 and draws a new picture
	std::unique_ptr<Video> newVideo(new Video());
	std::unique_ptr<Cpu> newCpu(new Cpu(newVideo.get()));
	memcpy(newCpu->getROM(), rom, sizeof(rom));
	newCpu->setFusex(fusex); // CF: the carry is an input of ADD and SUB
	sound->reset();
	newCpu->setSound(sound.get());

	// the old CPU goes before the video it points to
	cpu = std::move(newCpu);
	video = std::move(newVideo);

	// A new program has seen none of the taps that are still waiting: let them go, or the
	// buttons would stay pressed for good.
	joystick &= ~releaseDeferred;
	releaseDeferred = 0;
	pressedUnseen = 0;
	passJoystickToCpu();
	return true;
}

bool Machine::isLoaded() const
{
	return cpu.get() != NULL;
}

void Machine::reset()
{
	if (cpu.get() != NULL)
	{
		cpu->reset();
		video->reset();
		sound->reset();
	}
}

void Machine::runFrame()
{
	if (cpu.get() == NULL)
	{
		return;
	}
	cpu->runCycles(CYCLES_PER_FRAME);

	// a frame has seen the buttons: what was released meanwhile can go now
	joystick &= ~releaseDeferred;
	releaseDeferred = 0;
	pressedUnseen = 0;
	passJoystickToCpu();
}

void Machine::setJoystick(unsigned short mask, bool down)
{
	if (down)
	{
		joystick |= mask;
		pressedUnseen |= mask;
		releaseDeferred &= ~mask;
	}
	else if (pressedUnseen & mask)
	{
		// On a slow device a whole tap can arrive between two emulated frames.
		// The program only reads the joystick once per frame.
		releaseDeferred |= mask;
	}
	else
	{
		joystick &= ~mask;
	}
	passJoystickToCpu();
}

unsigned short Machine::getJoystick() const
{
	return joystick;
}

const Video& Machine::getVideo() const
{
	return *video;
}

Bu8763& Machine::getSound()
{
	return *sound;
}

void Machine::passJoystickToCpu()
{
	if (cpu.get() != NULL)
	{
		*cpu->getJoystrickr() = joystick;
	}
}
