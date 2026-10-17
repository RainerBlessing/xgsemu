#include <gtest/gtest.h>
#include <cmath>
#include <vector>
#include "../src/Bu8763.h"

namespace {

const unsigned char SCLK = 1 << 3;
const unsigned char SDATA = 1 << 4;
const unsigned char SSTB = 1 << 5;
const unsigned char CKSEL = 1 << 6;

const int SAMPLE_RATE = 44100;

class Bu8763Test : public ::testing::Test {
protected:
    Bu8763 chip;
    unsigned char ra = 0;

    // Clocks one bit in: SDATA is set up while SCLK is low, then SCLK rises.
    void clockBit(int bit) {
        ra = (ra & ~(SCLK | SDATA)) | (bit ? SDATA : 0);
        chip.portWrite(ra);
        ra |= SCLK;
        chip.portWrite(ra);
    }

    void strobe() {
        ra |= SSTB;
        chip.portWrite(ra);
        ra &= ~SSTB;
        chip.portWrite(ra);
    }

    void clockCommand(int address, int data) {
        int command = (address & 7) << 8 | (data & 0xff);
        for (int bit = 10; bit >= 0; bit--) {
            clockBit((command >> bit) & 1);
        }
    }

    // Datasheet order: 11 bits, then SSTB applies them.
    void writeRegister(int address, int data) {
        clockCommand(address, data);
        strobe();
    }

    std::vector<short> render(int samples) {
        std::vector<short> buffer(samples);
        chip.render(buffer.data(), samples, SAMPLE_RATE);
        return buffer;
    }

    static int peak(const std::vector<short>& buffer) {
        int result = 0;
        for (short sample : buffer) {
            result = std::max(result, std::abs((int)sample));
        }
        return result;
    }

    static int risingZeroCrossings(const std::vector<short>& buffer) {
        int count = 0;
        for (size_t i = 1; i < buffer.size(); i++) {
            if (buffer[i - 1] <= 0 && buffer[i] > 0) {
                count++;
            }
        }
        return count;
    }

    // Wave 1 on, square, no envelope, no attenuation, volume 0 dB.
    void playSquare(int note) {
        writeRegister(0, 0x80);
        writeRegister(1, 0x00);
        writeRegister(3, 0x00);
        writeRegister(2, note);
    }
};

TEST_F(Bu8763Test, StartsInSleepMode) {
    EXPECT_EQ(0x01, chip.oper());
}

TEST_F(Bu8763Test, CommandIsLatchedOnStrobe) {
    clockCommand(0, 0xe0);
    EXPECT_EQ(0x01, chip.oper());
    strobe();
    EXPECT_EQ(0xe0, chip.oper());
}

TEST_F(Bu8763Test, ModeRegisterIsStoredPerChannel) {
    writeRegister(1, 0x40 | 0x20 | 0x03);  // wave 2: sine, attenuation -8.5 dB
    EXPECT_EQ(0x23, chip.mode(1));
}

TEST_F(Bu8763Test, FrequencyRegisterUsesSixBitsAndChannelSelect) {
    writeRegister(2, 0x80 | 0x3f);  // wave 3, highest note
    EXPECT_EQ(0x3f, chip.freq(2));
}

TEST_F(Bu8763Test, GainRegisterIsStored) {
    writeRegister(3, 0x05);
    EXPECT_EQ(0x05, chip.gain());
}

TEST_F(Bu8763Test, StrobeBeforeBitsAppliesPreviousCommand) {
    // The Rem-Pac driver strobes first and clocks the bits afterwards.
    strobe();
    clockCommand(3, 0x02);
    EXPECT_EQ(0x00, chip.gain());
    strobe();
    EXPECT_EQ(0x02, chip.gain());
}

TEST_F(Bu8763Test, OnlyLast11BitsBeforeStrobeCount) {
    for (int i = 0; i < 5; i++) {
        clockBit(1);
    }
    clockCommand(3, 0x07);
    strobe();
    EXPECT_EQ(0x07, chip.gain());
}

TEST_F(Bu8763Test, NoteFrequenciesFollowEqualTemperamentFromA2) {
    EXPECT_NEAR(110.0, Bu8763::noteFrequency(0x00, false), 0.01);
    EXPECT_NEAR(440.0, Bu8763::noteFrequency(0x18, false), 0.01);
    EXPECT_NEAR(4186.0, Bu8763::noteFrequency(0x3f, false), 1.0);
}

TEST_F(Bu8763Test, HalfMasterClockLowersNoteByOneOctave) {
    EXPECT_NEAR(220.0, Bu8763::noteFrequency(0x18, true), 0.01);
}

TEST_F(Bu8763Test, SleepModeIsSilent) {
    writeRegister(0, 0x81);  // wave 1 on but sleep on
    writeRegister(2, 0x18);
    EXPECT_EQ(0, peak(render(1000)));
}

TEST_F(Bu8763Test, SquareWavePlaysRequestedPitch) {
    playSquare(0x18);
    EXPECT_NEAR(440, risingZeroCrossings(render(SAMPLE_RATE)), 2);
}

TEST_F(Bu8763Test, ClockSelectHighPlaysOneOctaveLower) {
    playSquare(0x18);
    ra |= CKSEL;
    chip.portWrite(ra);
    EXPECT_NEAR(220, risingZeroCrossings(render(SAMPLE_RATE)), 2);
}

TEST_F(Bu8763Test, DisabledChannelIsSilent) {
    playSquare(0x18);
    writeRegister(0, 0x00);
    EXPECT_EQ(0, peak(render(1000)));
}

TEST_F(Bu8763Test, SineWaveReachesSamePeakButIsNotSquare) {
    playSquare(0x18);
    int squarePeak = peak(render(2000));
    writeRegister(1, 0x20);
    std::vector<short> sine = render(2000);
    EXPECT_NEAR(squarePeak, peak(sine), squarePeak * 0.05);
    int nearPeak = 0;
    for (short sample : sine) {
        if (std::abs(sample) > squarePeak * 0.95) {
            nearPeak++;
        }
    }
    // A sine is above 95% of its peak about 20% of the time, a square always.
    EXPECT_LT(nearPeak, 600);
}

TEST_F(Bu8763Test, AttenuationOfSixDbHalvesTheLevel) {
    playSquare(0x18);
    int full = peak(render(2000));
    writeRegister(1, 0x02);
    EXPECT_NEAR(full / 2.0, peak(render(2000)), full * 0.03);
}

TEST_F(Bu8763Test, GainRegisterScalesAllChannels) {
    playSquare(0x18);
    int full = peak(render(2000));
    writeRegister(3, 0x03);  // -6 dB
    EXPECT_NEAR(full / 2.0, peak(render(2000)), full * 0.03);
}

TEST_F(Bu8763Test, EnvelopeDecaysAfterNoteStart) {
    writeRegister(0, 0x80);
    writeRegister(1, 0x10);  // envelope on, slow
    writeRegister(3, 0x00);
    writeRegister(2, 0x18);
    int start = peak(render(SAMPLE_RATE / 10));
    render(SAMPLE_RATE);
    int later = peak(render(SAMPLE_RATE / 10));
    EXPECT_LT(later, start / 2);
}

TEST_F(Bu8763Test, FastEnvelopeDecaysFasterThanSlow) {
    writeRegister(0, 0x80);
    writeRegister(3, 0x00);
    writeRegister(1, 0x10);
    writeRegister(2, 0x18);
    render(SAMPLE_RATE / 2);
    int slow = peak(render(SAMPLE_RATE / 10));

    writeRegister(1, 0x18);
    writeRegister(2, 0x18);
    render(SAMPLE_RATE / 2);
    int fast = peak(render(SAMPLE_RATE / 10));
    EXPECT_LT(fast, slow);
}

TEST_F(Bu8763Test, ResetReturnsToSleep) {
    writeRegister(0, 0xe0);
    chip.reset();
    EXPECT_EQ(0x01, chip.oper());
}

}  // namespace
