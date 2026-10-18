#include <gtest/gtest.h>
#include <string.h>
#include <vector>
#include "../src/Bu8763.h"
#include "../src/Machine.h"
#include "../src/Video.h"
#include "test_file.h"

#ifndef EXAMPLES_DIR
#define EXAMPLES_DIR "examples"
#endif

namespace {

const std::string STARFIELD = EXAMPLES_DIR "/starfield_01.hex";
const std::string REM_PAC = EXAMPLES_DIR "/rem_pac_01.hex";

bool anythingDrawn(const Video& video) {
    const uint32_t* pixels = video.pixels();
    for (int i = 0; i < video.width() * video.height(); i++) {
        if (pixels[i] != 0) {
            return true;
        }
    }
    return false;
}

std::vector<uint32_t> picture(const Machine& machine) {
    const Video& video = machine.getVideo();
    return std::vector<uint32_t>(video.pixels(), video.pixels() + video.width() * video.height());
}

void run(Machine& machine, int frames) {
    for (int i = 0; i < frames; i++) {
        machine.runFrame();
    }
}

}  // namespace

TEST(MachineTest, WithoutAProgramNothingHappensAndThePictureIsBlack) {
    Machine machine;
    EXPECT_FALSE(machine.isLoaded());
    run(machine, 3);
    machine.reset();
    EXPECT_FALSE(anythingDrawn(machine.getVideo()));
}

TEST(MachineTest, ALoadedProgramDrawsItsPicture) {
    Machine machine;
    ASSERT_TRUE(machine.load(STARFIELD));
    EXPECT_TRUE(machine.isLoaded());
    run(machine, 60);
    EXPECT_TRUE(anythingDrawn(machine.getVideo()));
}

TEST(MachineTest, AFileThatCannotBeReadChangesNothing) {
    Machine machine;
    EXPECT_FALSE(machine.load("no_such_file.hex"));
    EXPECT_FALSE(machine.isLoaded());

    ASSERT_TRUE(machine.load(STARFIELD));
    run(machine, 60);
    std::vector<uint32_t> before = picture(machine);
    ASSERT_TRUE(anythingDrawn(machine.getVideo()));

    TestFile broken(".hex", ":ZZ00000000\n");
    EXPECT_FALSE(machine.load(broken.name()));
    EXPECT_TRUE(machine.isLoaded());
    EXPECT_EQ(before, picture(machine));
}

TEST(MachineTest, ANewProgramStartsWithABlankPicture) {
    Machine machine;
    ASSERT_TRUE(machine.load(STARFIELD));
    run(machine, 60);
    ASSERT_TRUE(anythingDrawn(machine.getVideo()));
    ASSERT_TRUE(machine.load(STARFIELD));
    EXPECT_FALSE(anythingDrawn(machine.getVideo()));
}

// Everything is deterministic: a reset gives the same picture as a fresh start.
TEST(MachineTest, ResetStartsTheProgramFromTheBeginning) {
    Machine restarted;
    ASSERT_TRUE(restarted.load(STARFIELD));
    run(restarted, 7);
    restarted.reset();
    run(restarted, 60);

    Machine fresh;
    ASSERT_TRUE(fresh.load(STARFIELD));
    run(fresh, 60);

    EXPECT_EQ(picture(fresh), picture(restarted));
}

TEST(MachineTest, ALoadedProgramPlaysThroughTheSoundChipOfTheMachine) {
    Machine machine;
    ASSERT_TRUE(machine.load(REM_PAC));
    run(machine, 30);
    EXPECT_EQ(0xc0, machine.getSound().oper());   // Rem-Pac wakes the chip and turns on two waves

    ASSERT_TRUE(machine.load(STARFIELD));        // a program without sound: the chip is reset
    EXPECT_EQ(0x01, machine.getSound().oper());
}

class MachineJoystickTest : public ::testing::Test {
protected:
    Machine machine;

    void SetUp() override {
        ASSERT_TRUE(machine.load(STARFIELD));
    }
};

TEST_F(MachineJoystickTest, APressedButtonIsSeenByTheProgram) {
    machine.setJoystick(0x10, true);
    EXPECT_EQ(0x10, machine.getJoystick());
    machine.setJoystick(0x01, true);
    EXPECT_EQ(0x11, machine.getJoystick());
}

TEST_F(MachineJoystickTest, AButtonThatAFrameHasSeenIsReleasedAtOnce) {
    machine.setJoystick(0x10, true);
    machine.runFrame();
    machine.setJoystick(0x10, false);
    EXPECT_EQ(0, machine.getJoystick());
}

// A tap that arrives completely between two frames would never be seen otherwise.
TEST_F(MachineJoystickTest, AButtonReleasedBeforeAnyFrameSawItStaysUntilTheFrameIsOver) {
    machine.setJoystick(0x10, true);
    machine.setJoystick(0x10, false);
    EXPECT_EQ(0x10, machine.getJoystick());
    machine.runFrame();
    EXPECT_EQ(0, machine.getJoystick());
}

TEST_F(MachineJoystickTest, PressingAgainCancelsTheWaitingRelease) {
    machine.setJoystick(0x10, true);
    machine.setJoystick(0x10, false);
    machine.setJoystick(0x10, true);
    machine.runFrame();
    EXPECT_EQ(0x10, machine.getJoystick());
}

TEST_F(MachineJoystickTest, ButtonsAreIndependent) {
    machine.setJoystick(0x01, true);
    machine.setJoystick(0x02, true);
    machine.runFrame();
    machine.setJoystick(0x01, false);
    EXPECT_EQ(0x02, machine.getJoystick());
}

TEST_F(MachineJoystickTest, AHeldButtonStaysPressedWhenAnotherProgramIsLoaded) {
    machine.setJoystick(0x08, true);
    machine.runFrame();
    ASSERT_TRUE(machine.load(REM_PAC));
    EXPECT_EQ(0x08, machine.getJoystick());
}

TEST(MachineJoystickWithoutProgramTest, ATapBeforeTheProgramIsNotStuckAfterLoading) {
    Machine machine;
    machine.setJoystick(0x10, true);
    machine.setJoystick(0x10, false);
    ASSERT_TRUE(machine.load(STARFIELD));
    EXPECT_EQ(0, machine.getJoystick());
}
