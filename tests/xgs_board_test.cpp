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

// The SRAM pins driven directly, one register write at a time: RC bit 4 is the clock of the
// page register (74HCT164: it takes RC5 with the rising edge), RC6 the chip enable (low:
// enabled), RC7 the direction (low: write), RD the data, RC0..RC3 the lowest address bits.
class XgsBoardSramPinsTest : public XgsBoardTest {
protected:
    static const unsigned char CLOCK = 0x10, DATA = 0x20, DISABLED = 0x40, READ = 0x80;

    void writeRc(unsigned char value) {
        registers[2] = value;
        board.portWritten(2, registers, 0);
    }

    // one pulse of the clock with the bit on RC5; the chip stays disabled
    void clockBit(int bit) {
        writeRc(DISABLED | (bit ? DATA : 0) | CLOCK);
        writeRc(DISABLED | (bit ? DATA : 0));
    }

    // the 12 bit page, most significant bit first
    void setPage(int page) {
        for (int bit = 11; bit >= 0; bit--) {
            clockBit((page >> bit) & 1);
        }
    }

    void writeByte(int low, unsigned char value) {
        registers[3] = value;
        writeRc(low);                                 // enabled, write: stored
        writeRc(DISABLED | low);
    }

    unsigned char readByte(int low) {
        writeRc(READ | low);                          // enabled, read: RD shows the byte
        unsigned char value = registers[3];
        writeRc(DISABLED | low);
        return value;
    }
};

TEST_F(XgsBoardSramPinsTest, TheAddressIsThePageAndTheLowFourBits) {
    setPage(0x123);
    writeByte(0x5, 0xab);
    EXPECT_EQ(0xab, board.getSRAM()[(0x123 << 4) | 0x5]);
}

TEST_F(XgsBoardSramPinsTest, WhatWasWrittenCanBeReadFromTheSameAddress) {
    setPage(0x042);
    writeByte(0x7, 0x3c);
    registers[3] = 0;
    EXPECT_EQ(0x3c, readByte(0x7));
    EXPECT_EQ(0, readByte(0x8));                      // another address
}

// A 74HCT164 takes its data with the rising edge only.
TEST_F(XgsBoardSramPinsTest, TheClockShiftsOnlyWithTheRisingEdge) {
    setPage(0x001);
    writeRc(DISABLED | CLOCK);                        // rising edge, data 0: the page is 0x002
    writeRc(DISABLED | CLOCK | DATA);                 // the clock stays high: nothing is shifted
    writeRc(DISABLED | CLOCK);
    writeRc(DISABLED);                                // falling edge: nothing is shifted
    writeByte(0x0, 0x11);
    EXPECT_EQ(0x11, board.getSRAM()[0x002 << 4]);
}

// An instruction that changes RC without being a write (or, and, add...) clocks as well.
TEST_F(XgsBoardSramPinsTest, AnyChangeOfTheClockPinCountsNotOnlyWritesToRc) {
    registers[2] = DISABLED | DATA | CLOCK;
    board.portChanged(2, registers);                  // no portWritten: "or rc,w" did it
    registers[2] = DISABLED;
    board.portChanged(2, registers);
    writeByte(0x0, 0x22);
    EXPECT_EQ(0x22, board.getSRAM()[0x001 << 4]);
}

TEST_F(XgsBoardSramPinsTest, TheSameClockLevelSeenTwiceIsOneEdge) {
    registers[2] = DISABLED | DATA | CLOCK;
    board.portWritten(2, registers, 0);               // the instruction that wrote it
    board.portChanged(2, registers);                  // and the report before the next one
    registers[2] = DISABLED;
    board.portWritten(2, registers, 0);
    board.portChanged(2, registers);
    writeByte(0x0, 0x33);
    EXPECT_EQ(0x33, board.getSRAM()[0x001 << 4]);
}

// Nothing clears the shift registers: their reset pins are tied high.
TEST_F(XgsBoardSramPinsTest, ThePageStaysWhenTheChipIsDisabledAndEnabledAgain) {
    setPage(0x0f0);
    writeByte(0x0, 0x44);
    writeByte(0x1, 0x55);
    EXPECT_EQ(0x44, board.getSRAM()[0x0f0 << 4]);
    EXPECT_EQ(0x55, board.getSRAM()[(0x0f0 << 4) | 1]);
}

TEST_F(XgsBoardSramPinsTest, OnlyTheLastTwelveBitsMakeThePage) {
    for (int i = 0; i < 16; i++) {
        clockBit(1);
    }
    writeByte(0x0, 0x66);
    EXPECT_EQ(0x66, board.getSRAM()[0xfff << 4]);
}

TEST_F(XgsBoardSramPinsTest, Ra7IsTheHighestAddressBit) {
    setPage(0x001);
    registers[0] = 0x80;
    writeByte(0x0, 0x77);
    EXPECT_EQ(0x77, board.getSRAM()[0x10000 | (0x001 << 4)]);
    EXPECT_EQ(0, board.getSRAM()[0x001 << 4]);
}

TEST_F(XgsBoardSramPinsTest, ADisabledChipStoresNothing) {
    setPage(0x010);
    registers[3] = 0x88;
    writeRc(DISABLED | 0x0);
    writeRc(DISABLED | 0x1);
    board.portWritten(3, registers, 0);
    EXPECT_EQ(0, board.getSRAM()[0x010 << 4]);
    EXPECT_EQ(0, board.getSRAM()[(0x010 << 4) | 1]);
}

// The data bus is the pins of RD: a byte that appears there while the chip writes is stored.
TEST_F(XgsBoardSramPinsTest, ANewByteOnRdWhileTheChipWritesIsStored) {
    setPage(0x020);
    registers[3] = 0;
    writeRc(0x2);                                     // enabled for writing, RD is 0
    registers[3] = 0x99;
    board.portWritten(3, registers, 0);               // mov rd,w
    EXPECT_EQ(0x99, board.getSRAM()[(0x020 << 4) | 2]);
    registers[3] = 0xaa;
    board.portChanged(3, registers);                  // any other instruction
    EXPECT_EQ(0xaa, board.getSRAM()[(0x020 << 4) | 2]);
}

TEST_F(XgsBoardSramPinsTest, RdIsNotStoredWhileTheChipReads) {
    setPage(0x020);
    writeByte(0x2, 0x12);
    writeRc(READ | 0x2);                              // reading: RD shows 0x12
    registers[3] = 0x34;                              // someone else drives the pins
    board.portWritten(3, registers, 0);
    EXPECT_EQ(0x12, board.getSRAM()[(0x020 << 4) | 2]);
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
    ram[261] = 0x40 | 0x03;   // chip disabled
    run(0x027);               // mov RC,w
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
