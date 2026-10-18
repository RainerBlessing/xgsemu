#include "Machine.h"
#include <string.h>
#include "Bu8763.h"
#include "Video.h"
#include "cpu.h"
#include "XgsBoard.h"
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
	// The members go in reverse order: the CPU first, it points to the board, and the board
	// points to the video and the sound chip.
}

bool Machine::load(const std::string& path, std::string* error)
{
	// Read the file first: if it cannot be read the running program stays as it is
	u16 rom[4096];
	u16 fusex;
	if (!FileHandler::readFile(path.c_str(), rom, &fusex, error))
	{
		return false;
	}

	// a new video and a new board: the program counts cycles from 0, draws a new picture and
	// finds an empty SRAM
	std::unique_ptr<Video> newVideo(new Video());
	std::unique_ptr<XgsBoard> newBoard(new XgsBoard(newVideo.get(), sound.get()));
	std::unique_ptr<Cpu> newCpu(new Cpu(newBoard.get()));
	memcpy(newCpu->getROM(), rom, sizeof(rom));
	newCpu->setFusex(fusex); // CF: the carry is an input of ADD and SUB
	sound->reset();

	// the old CPU goes before the board it points to, the board before the video
	cpu = std::move(newCpu);
	board = std::move(newBoard);
	video = std::move(newVideo);

	// A new program has seen none of the taps that are still waiting: let them go, or the
	// buttons would stay pressed for good.
	joystick &= ~releaseDeferred;
	releaseDeferred = 0;
	pressedUnseen = 0;
	passJoystickToBoard();
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
		board->reset();
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
	passJoystickToBoard();
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
	passJoystickToBoard();
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

void Machine::passJoystickToBoard()
{
	if (board.get() != NULL)
	{
		board->setJoystick(joystick);
	}
}
