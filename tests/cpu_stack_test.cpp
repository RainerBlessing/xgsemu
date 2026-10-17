#include <gtest/gtest.h>
#include <vector>
#include "../src/Video.h"
#include "cpu.h"

// The SX52 has a hardware stack of eight return addresses. A ninth nested
// call wraps around and overwrites the oldest entry.
class StackTest : public ::testing::Test {
protected:
    enum {
        NOP = 0x000,
        CALL_BASE = 0x900,
        RETW_BASE = 0x800
    };
    static const int STEP = 10;     // distance between the nested calls

    Video video;

    // The call at STEP * k jumps to STEP * (k + 1); the return address
    // STEP * k + 1 holds a "retw". The deepest level is a "retw" as well.
    // Runs `depth` calls, then as many returns, and gives the pc after each
    // return.
    std::vector<int> callAndReturn(int depth) {
        Cpu cpu(&video);
        USHORT* rom = cpu.getROM();
        for (int i = 0; i < 256; i++) {
            rom[i] = NOP;
        }
        for (int k = 0; k < depth; k++) {
            rom[STEP * k] = CALL_BASE | (STEP * (k + 1));
            rom[STEP * k + 1] = RETW_BASE;
        }
        rom[STEP * depth] = RETW_BASE;
        *cpu.getPC() = 0;

        for (int i = 0; i < depth; i++) {
            cpu.execute();
        }
        std::vector<int> returns;
        for (int i = 0; i < depth; i++) {
            cpu.execute();
            returns.push_back(*cpu.getPC());
        }
        return returns;
    }
};

TEST_F(StackTest, EightNestedCallsReturnInReverseOrder) {
    std::vector<int> returns = callAndReturn(8);
    ASSERT_EQ(8u, returns.size());
    for (int i = 0; i < 8; i++) {
        EXPECT_EQ(STEP * (7 - i) + 1, returns[i]) << "return " << i;
    }
}

TEST_F(StackTest, NinthCallOverwritesTheOldestReturnAddress) {
    std::vector<int> returns = callAndReturn(9);
    ASSERT_EQ(9u, returns.size());
    for (int i = 0; i < 8; i++) {
        EXPECT_EQ(STEP * (8 - i) + 1, returns[i]) << "return " << i;
    }
    // the ninth return finds the address of the ninth call again
    EXPECT_EQ(STEP * 8 + 1, returns[8]);
}
