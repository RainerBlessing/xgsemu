#include <gtest/gtest.h>
#include "../src/Bu8763.h"
#include "../src/Video.h"
#include "cpu.h"
#include "../src/filehandler.h"

#ifndef EXAMPLES_DIR
#define EXAMPLES_DIR "examples"
#endif

// Runs a real program (Rem-Pac) and checks that its writes to port RA arrive
// at the sound chip.
class CpuSoundTest : public ::testing::Test {
protected:
    Video video;
    Bu8763 chip;
    Cpu cpu;

    CpuSoundTest() : cpu(&video) {}

    void runInstructions(int count) {
        for (int i = 0; i < count; i++) {
            cpu.execute();
        }
    }
};

TEST_F(CpuSoundTest, RemPacProgramsTheSoundChip) {
    ASSERT_TRUE(FileHandler::readFile(EXAMPLES_DIR "/rem_pac_01.hex", cpu.getROM()));
    cpu.setSound(&chip);
    // About 10 seconds of 80 MHz / 4 instructions at 60 frames per second.
    runInstructions(3000000);
    // init_sound_system enables wave 1 + 2 and leaves sleep mode.
    EXPECT_EQ(0xc0, chip.oper());
    // Wave 2 is a sine with envelope, set up in init_sound_system.
    EXPECT_EQ(0x20 | 0x03, chip.mode(1) & 0x2f);
}

TEST_F(CpuSoundTest, ProgramWithoutSoundLeavesChipInSleep) {
    ASSERT_TRUE(FileHandler::readFile(EXAMPLES_DIR "/starfield_01.hex", cpu.getROM()));
    cpu.setSound(&chip);
    runInstructions(1000000);
    EXPECT_EQ(0x01, chip.oper());
}
