#include <gtest/gtest.h>
#include <string.h>
#include "../src/XgsBoard.h"
#include "cpu.h"

namespace {

class RecordingVideo : public VideoPort {
public:
    void changeVideo(int cycle, unsigned char colour) override {
        calls++;
        lastCycle = cycle;
        lastColour = colour;
    }
    int calls = 0;
    int lastCycle = 0;
    unsigned char lastColour = 0;
};

class RecordingSound : public SoundPort {
public:
    void portWrite(unsigned char ra) override {
        calls++;
        lastRa = ra;
    }
    int calls = 0;
    unsigned char lastRa = 0;
};

}  // namespace

// The registers of the ports as the board sees them: RA..RE.
class XgsBoardTest : public ::testing::Test {
protected:
    RecordingVideo video;
    RecordingSound sound;
    XgsBoard board;
    unsigned char registers[5];

    XgsBoardTest() : board(&video, &sound) {
        memset(registers, 0, sizeof(registers));
    }

    // The program writes a value to RA and the board sees the write.
    void writeRa(unsigned char value) {
        registers[0] = value;
        board.portWritten(0, registers, 0);
    }

    // A program reads the 16 bits of the joystick the way the XGS ME programs do: RA1 low
    // starts, then one pulse of the clock RA0 per bit, the data is on RA2 (low: pressed).
    unsigned short readJoystick() {
        writeRa(0x00);
        unsigned short word = 0;
        for (int i = 0; i < 16; i++) {
            writeRa(0x03);                            // the clock is high
            writeRa(0x02);                            // it falls: the next bit is on RA2
            word = (word << 1) | ((registers[0] & 4) ? 0 : 1);
        }
        return word;
    }
};

TEST_F(XgsBoardTest, ColoursWrittenToPortREReachTheVideoWithTheCycle) {
    registers[4] = 0xa5;
    board.portWritten(4, registers, 1234);
    EXPECT_EQ(1, video.calls);
    EXPECT_EQ(1234, video.lastCycle);
    EXPECT_EQ(0xa5, video.lastColour);
}

TEST_F(XgsBoardTest, OtherPortsAreNotTheVideo) {
    board.portWritten(1, registers, 0);
    board.portWritten(3, registers, 0);
    EXPECT_EQ(0, video.calls);
}

TEST_F(XgsBoardTest, ThePortRaReachesTheSoundChipWhenItChanges) {
    registers[0] = 0x58;
    board.portChanged(0, registers);
    EXPECT_EQ(1, sound.calls);
    EXPECT_EQ(0x58, sound.lastRa);
    board.portChanged(1, registers);
    EXPECT_EQ(1, sound.calls);                        // only RA is the sound chip
}

TEST_F(XgsBoardTest, ABoardWithoutVideoAndSoundIsFine) {
    XgsBoard bare(NULL, NULL);
    bare.portWritten(4, registers, 0);
    bare.portChanged(0, registers);
    bare.portWritten(0, registers, 0);
}

TEST_F(XgsBoardTest, TheJoystickIsReadMostSignificantBitFirst) {
    board.setJoystick(0x8421);
    EXPECT_EQ(0x8421, readJoystick());
    board.setJoystick(0x0010);                        // fire
    EXPECT_EQ(0x0010, readJoystick());
    board.setJoystick(0);
    EXPECT_EQ(0, readJoystick());
}

TEST_F(XgsBoardTest, TheJoystickStartsAgainWhenRa1GoesLow) {
    board.setJoystick(0x8000);
    EXPECT_EQ(0x8000, readJoystick());
    EXPECT_EQ(0x8000, readJoystick());                // the second read starts from the first bit
}

TEST_F(XgsBoardTest, WhatTheProgramPutOnRa2IsReplacedByTheJoystickAtEveryPulse) {
    board.setJoystick(0xffff);
    writeRa(0x00);
    for (int i = 0; i < 16; i++) {
        writeRa(0x03 | 0x04);                         // the program drives RA2 high
        writeRa(0x02 | 0x04);
        EXPECT_EQ(0, registers[0] & 4) << "bit " << i;   // all buttons pressed: RA2 low
    }
}

TEST_F(XgsBoardTest, TheOtherBitsOfRaAreLeftAloneByTheJoystick) {
    board.setJoystick(0);                             // nothing pressed: RA2 goes high
    writeRa(0x00);
    writeRa(0xfb);                                    // the clock is high, RA2 low
    writeRa(0xfa);                                    // the clock falls
    EXPECT_EQ(0xfe, registers[0]);                    // only RA2 is different from what was written
}

TEST_F(XgsBoardTest, AClockPulseWithoutRisingIsNoShift) {
    board.setJoystick(0x8000);
    writeRa(0x00);                                    // RA1 low: first bit, the clock is low
    writeRa(0x00);                                    // low again: no falling edge
    EXPECT_EQ(0, registers[0] & 4);                   // nothing was put on RA2
}

// The SRAM is addressed through port C and D: bit 4 of RC clocks one bit (bit 5) into a 12 bit
// page register, bit 6 is the chip enable. The tests run a real CPU against the board.
class XgsBoardSramTest : public ::testing::Test {
protected:
    XgsBoard board;
    Cpu cpu;

    XgsBoardSramTest() : board(NULL, NULL), cpu(&board) {}

    void run(unsigned short opcode) {
        *cpu.getPC() = 0x100;
        cpu.getROM()[0x100] = opcode;
        cpu.execute();
    }

    void configureAllPortsAsOutputs() {
        cpu.getRAM()[261] = 0;
        run(0x05f);   // mode #0xf
        run(0x007);   // mov !rc,w
    }

    void clockPageBit(int bit) {
        unsigned char* rc = &cpu.getRAM()[7];
        *rc = (*rc & ~0x20) | (bit << 5);
        run(0x587);   // setb RC.4
        run(0x487);   // clrb RC.4
    }
};

// Characterisation: only the low 12 bits of the page take part in the address.
TEST_F(XgsBoardSramTest, OnlyTheLast12ClockedBitsFormThePage) {
    configureAllPortsAsOutputs();
    unsigned char* ram = cpu.getRAM();
    ram[7] = 1 << 6;          // chip disabled
    for (int i = 0; i < 20; i++) {
        clockPageBit(1);      // more than 12 bits: the oldest ones fall out
    }
    ram[8] = 0x5a;            // RD, the data
    ram[7] = (ram[7] & ~0x0f) | 0x03;
    ram[7] &= ~(1 << 7);      // write
    run(0x4c7);               // clrb RC.6: chip enable, writes the SRAM

    EXPECT_EQ(0x5a, board.getSRAM()[(0xfff << 4) | 0x3]);
}

// RA7 is the highest address bit of the 128 KB SRAM.
TEST_F(XgsBoardSramTest, Ra7SelectsTheUpperHalfOfTheSram) {
    configureAllPortsAsOutputs();
    unsigned char* ram = cpu.getRAM();
    ram[7] = 1 << 6;          // chip disabled
    for (int i = 0; i < 12; i++) {
        clockPageBit(1);
    }
    ram[5] |= 1 << 7;         // RA7
    ram[8] = 0x5a;
    ram[7] = (ram[7] & ~0x0f) | 0x03;
    ram[7] &= ~(1 << 7);      // write
    run(0x4c7);               // clrb RC.6: chip enable, writes the SRAM

    EXPECT_EQ(0x5a, board.getSRAM()[0x10000 | (0xfff << 4) | 0x3]);
    EXPECT_EQ(0x00, board.getSRAM()[(0xfff << 4) | 0x3]);
}

// The SRAM sees every write to port C, not only setb and clrb.
TEST_F(XgsBoardSramTest, MovRcWithChipEnableLowWritesTheSram) {
    configureAllPortsAsOutputs();
    unsigned char* ram = cpu.getRAM();
    ram[8] = 0x5a;            // RD, the data
    ram[261] = 0x03;          // W: chip enable 0, write, low address bits 3
    run(0x027);               // mov RC,w

    EXPECT_EQ(0x5a, board.getSRAM()[0x3]);
}

TEST_F(XgsBoardSramTest, WhatWasWrittenCanBeReadBack) {
    configureAllPortsAsOutputs();
    unsigned char* ram = cpu.getRAM();
    ram[8] = 0x5a;
    ram[261] = 0x03;
    run(0x027);               // mov RC,w: write at address 3
    ram[8] = 0;
    ram[261] = 0x80 | 0x03;   // chip enable 0, read
    run(0x027);               // mov RC,w
    EXPECT_EQ(0x5a, ram[8]);
}

TEST_F(XgsBoardSramTest, ResetEmptiesTheSram) {
    configureAllPortsAsOutputs();
    cpu.getRAM()[8] = 0x5a;
    cpu.getRAM()[261] = 0x03;
    run(0x027);
    ASSERT_EQ(0x5a, board.getSRAM()[3]);
    board.reset();
    EXPECT_EQ(0, board.getSRAM()[3]);
}
