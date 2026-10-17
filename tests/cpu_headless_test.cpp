#include <gtest/gtest.h>
#include "cpu.h"

// A front end without a screen, like the debugger of sxemu, passes no video.
TEST(CpuHeadlessTest, WritingPortREWithoutVideoDoesNotCrash) {
    Cpu cpu(NULL);
    cpu.reset();
    cpu.getROM()[0xfff] = 0xfff;
    cpu.getROM()[0] = 0xc01;   // mov w,#1
    cpu.getROM()[1] = 0x029;   // mov re,w (port RE is register 9)
    *cpu.getPC() = 0;
    cpu.execute();
    cpu.execute();
    EXPECT_EQ(1, cpu.getRAM()[9]);
}

TEST(CpuHeadlessTest, ModeIs1FAfterReset) {
    Cpu cpu(NULL);
    cpu.reset();
    EXPECT_EQ(0x1f, *cpu.getMode());
}
