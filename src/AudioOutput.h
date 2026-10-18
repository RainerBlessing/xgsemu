#ifndef AUDIOOUTPUT_H_
#define AUDIOOUTPUT_H_

#include <SDL.h>

class Bu8763;

/**
 * Plays a sound chip through SDL. SDL calls back from its own audio thread, the chip is
 * made for that (see Bu8763). Without an audio device the emulator just runs silently.
 */
class AudioOutput
{
public:
	explicit AudioOutput(Bu8763& sound);
	/** Closes the device: the audio thread is gone when this returns. */
	~AudioOutput();

	/** Opens the default device and starts playing. @return false if there is none */
	bool open();
	void close();
	void setMuted(bool muted);

private:
	AudioOutput(const AudioOutput&) = delete;
	AudioOutput& operator=(const AudioOutput&) = delete;

	static void callback(void* userdata, Uint8* stream, int length);

	Bu8763& sound;
	SDL_AudioDeviceID device;
	int sampleRate;
};

#endif /*AUDIOOUTPUT_H_*/
