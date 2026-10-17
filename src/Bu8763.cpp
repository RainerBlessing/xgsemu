#include <cmath>
#include "Bu8763.h"

namespace
{
const unsigned char RA_SCLK = 1 << 3;
const unsigned char RA_SDATA = 1 << 4;
const unsigned char RA_SSTB = 1 << 5;
const unsigned char RA_CKSEL = 1 << 6;

const int COMMAND_BITS = 11;

const int REG_OPER = 0;
const int REG_MODE = 1;
const int REG_FREQ = 2;
const int REG_GAIN = 3;

const unsigned char OPER_SLEEP = 0x01;
const unsigned char OPER_WAVE1 = 0x80;
const unsigned char MODE_SINE = 0x20;
const unsigned char MODE_ENVELOPE_ON = 0x10;
const unsigned char MODE_ENVELOPE_FAST = 0x08;
const unsigned char MODE_ATTENUATION_MASK = 0x07;

const double LOWEST_NOTE_HZ = 110.0;
const double CHANNEL_LEVEL = 0.25;
const double SLOW_ENVELOPE_SECONDS = 1.6;
const double FAST_ENVELOPE_SECONDS = 0.8;
// The envelope fades out by this much within the envelope time.
const double ENVELOPE_RANGE_DB = 60.0;
const double TWO_PI = 6.283185307179586;

const double ATTENUATION_DB[8] = {0.0, 2.5, 6.0, 8.5, 12.0, 14.5, 18.0, 24.0};

double decibelToFactor(double db)
{
	return pow(10.0, -db / 20.0);
}
}

Bu8763::Bu8763()
{
	reset();
}

void Bu8763::reset()
{
	std::lock_guard<std::mutex> guard(lock);
	shift = 0;
	lastSclk = false;
	lastSstb = false;
	operReg = OPER_SLEEP;
	modeReg[0] = 0x01;
	modeReg[1] = 0x04;
	modeReg[2] = 0x00;
	for (int i = 0; i < CHANNELS; i++)
	{
		freqReg[i] = 0;
		phase[i] = 0.0;
		envelope[i] = 1.0;
	}
	gainReg = 0;
	sineWave = false;
	halfClock = false;
}

void Bu8763::portWrite(unsigned char ra)
{
	bool sclk = (ra & RA_SCLK) != 0;
	bool sstb = (ra & RA_SSTB) != 0;
	if (sclk && !lastSclk)
	{
		shift = ((shift << 1) | ((ra & RA_SDATA) ? 1 : 0)) & ((1 << COMMAND_BITS) - 1);
	}
	if (sstb && !lastSstb)
	{
		latch(shift);
	}
	lastSclk = sclk;
	lastSstb = sstb;

	std::lock_guard<std::mutex> guard(lock);
	halfClock = (ra & RA_CKSEL) != 0;
}

void Bu8763::latch(int command)
{
	int address = (command >> 8) & 7;
	unsigned char data = command & 0xff;
	int channel = data >> 6;

	std::lock_guard<std::mutex> guard(lock);
	switch (address)
	{
		case REG_OPER:
			operReg = data;
			break;
		case REG_MODE:
			if (channel < CHANNELS)
			{
				modeReg[channel] = data & 0x3f;
				// the data sheet: the wave form is the same for WAVE1, 2 and 3
				sineWave = (data & MODE_SINE) != 0;
			}
			break;
		case REG_FREQ:
			if (channel < CHANNELS)
			{
				freqReg[channel] = data & 0x3f;
				envelope[channel] = 1.0; // a new note restarts the envelope
			}
			break;
		case REG_GAIN:
			gainReg = data & 0x0f;
			break;
		default:
			break;
	}
}

double Bu8763::noteFrequency(int index, bool halfClock)
{
	double frequency = LOWEST_NOTE_HZ * pow(2.0, (index & 0x3f) / 12.0);
	return halfClock ? frequency / 2.0 : frequency;
}

void Bu8763::render(short* out, int samples, int sampleRate)
{
	std::lock_guard<std::mutex> guard(lock);
	bool awake = (operReg & OPER_SLEEP) == 0;
	// Volume values 8..15 are reserved, treat them like the lowest level.
	double master = decibelToFactor(2.0 * (gainReg > 7 ? 7 : gainReg));

	double step[CHANNELS];
	double level[CHANNELS];
	double decay[CHANNELS];
	bool active[CHANNELS];
	for (int c = 0; c < CHANNELS; c++)
	{
		active[c] = awake && (operReg & (OPER_WAVE1 >> c)) != 0;
		step[c] = noteFrequency(freqReg[c], halfClock) / sampleRate;
		level[c] = CHANNEL_LEVEL * master * decibelToFactor(ATTENUATION_DB[modeReg[c] & MODE_ATTENUATION_MASK]);
		double seconds = (modeReg[c] & MODE_ENVELOPE_FAST) ? FAST_ENVELOPE_SECONDS : SLOW_ENVELOPE_SECONDS;
		decay[c] = decibelToFactor(ENVELOPE_RANGE_DB / (seconds * sampleRate));
	}

	for (int i = 0; i < samples; i++)
	{
		double mix = 0.0;
		for (int c = 0; c < CHANNELS; c++)
		{
			phase[c] += step[c];
			phase[c] -= floor(phase[c]);
			if (modeReg[c] & MODE_ENVELOPE_ON)
			{
				envelope[c] *= decay[c];
			}
			else
			{
				envelope[c] = 1.0;
			}
			if (!active[c])
			{
				continue;
			}
			double wave;
			if (sineWave)
			{
				wave = sin(TWO_PI * phase[c]);
			}
			else
			{
				wave = phase[c] < 0.5 ? 1.0 : -1.0;
			}
			mix += wave * level[c] * envelope[c];
		}
		out[i] = (short)(mix * 32767.0);
	}
}

unsigned char Bu8763::oper() const
{
	std::lock_guard<std::mutex> guard(lock);
	return operReg;
}

unsigned char Bu8763::mode(int channel) const
{
	std::lock_guard<std::mutex> guard(lock);
	return modeReg[channel];
}

unsigned char Bu8763::freq(int channel) const
{
	std::lock_guard<std::mutex> guard(lock);
	return freqReg[channel];
}

unsigned char Bu8763::gain() const
{
	std::lock_guard<std::mutex> guard(lock);
	return gainReg;
}
