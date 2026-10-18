#include "AudioOutput.h"
#include <stdio.h>
#include "Bu8763.h"

AudioOutput::AudioOutput(Bu8763& sound)
	: sound(sound),
	  device(0),
	  sampleRate(44100)
{
}

AudioOutput::~AudioOutput()
{
	close();
}

bool AudioOutput::open()
{
	// Kein Audio-Geraet ist kein Grund, den Emulator nicht zu starten.
	if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
	{
		printf("Audio konnte nicht initialisiert werden! SDL-Fehler: %s\n", SDL_GetError());
		return false;
	}

	SDL_AudioSpec wanted;
	SDL_AudioSpec obtained;
	SDL_zero(wanted);
	wanted.freq = sampleRate;
	wanted.format = AUDIO_S16SYS;
	wanted.channels = 1;
#ifdef __EMSCRIPTEN__
	// Emulation and audio share one thread in the browser. A bigger buffer
	// (about 93 ms) survives frames that take longer than 1024 samples (23 ms).
	wanted.samples = 4096;
#else
	wanted.samples = 1024;
#endif
	wanted.callback = callback;
	wanted.userdata = this;

	device = SDL_OpenAudioDevice(NULL, 0, &wanted, &obtained, 0);
	if (device == 0)
	{
		printf("Audio-Geraet konnte nicht geoeffnet werden! SDL-Fehler: %s\n", SDL_GetError());
		return false;
	}
	// the device starts paused, so the callback has not seen the rate yet
	sampleRate = obtained.freq;
	SDL_PauseAudioDevice(device, 0);
	return true;
}

void AudioOutput::close()
{
	if (device != 0)
	{
		SDL_CloseAudioDevice(device);
		device = 0;
	}
}

void AudioOutput::setMuted(bool muted)
{
	if (device != 0)
	{
		SDL_PauseAudioDevice(device, muted ? 1 : 0);
	}
}

void AudioOutput::callback(void* userdata, Uint8* stream, int length)
{
	AudioOutput* output = static_cast<AudioOutput*>(userdata);
	output->sound.render(reinterpret_cast<short*>(stream), length / sizeof(short), output->sampleRate);
}
