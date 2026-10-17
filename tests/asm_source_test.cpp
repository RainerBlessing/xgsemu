#include <gtest/gtest.h>
#include <string>
#include "asm.h"

class AssembleSourceTest : public ::testing::Test {
protected:
    enum { ROM_SIZE = 4096, UNUSED = 0xfff };
    unsigned short rom[ROM_SIZE];
    std::string error;

    bool assembleText(const std::string& source) {
        return assembleSource(source, rom, ROM_SIZE, &error);
    }
};

TEST_F(AssembleSourceTest, PlacesInstructionsFromAddressZero) {
    ASSERT_TRUE(assembleText("mov w, #a\nnop\n")) << error;
    EXPECT_EQ(0xc0a, rom[0]);
    EXPECT_EQ(0x000, rom[1]);
}

TEST_F(AssembleSourceTest, LeavesTheRestUnused) {
    ASSERT_TRUE(assembleText("nop\n")) << error;
    EXPECT_EQ(UNUSED, rom[1]);
    EXPECT_EQ(UNUSED, rom[ROM_SIZE - 1]);
}

TEST_F(AssembleSourceTest, SkipsEmptyLinesAndComments) {
    ASSERT_TRUE(assembleText("; start\n\n  nop ; wait\n\t\nret\n")) << error;
    EXPECT_EQ(0x000, rom[0]);
    EXPECT_EQ(0x00c, rom[1]);
    EXPECT_EQ(UNUSED, rom[2]);
}

TEST_F(AssembleSourceTest, OrgSetsTheAddress) {
    ASSERT_TRUE(assembleText("org 100\nnop\n")) << error;
    EXPECT_EQ(UNUSED, rom[0]);
    EXPECT_EQ(0x000, rom[0x100]);
}

TEST_F(AssembleSourceTest, JumpToALabelBehindIt) {
    ASSERT_TRUE(assembleText("jmp done\nnop\ndone: nop\n")) << error;
    EXPECT_EQ(0xa00 | 2, rom[0]);
}

TEST_F(AssembleSourceTest, JumpToALabelBeforeIt) {
    ASSERT_TRUE(assembleText("loop:\n  inc 10\n  jmp loop\n")) << error;
    EXPECT_EQ(0xa00 | 0, rom[1]);
}

TEST_F(AssembleSourceTest, CallUsesTheLowEightBits) {
    ASSERT_TRUE(assembleText("org 142\nsub1: ret\norg 0\ncall sub1\n")) << error;
    EXPECT_EQ(0x900 | 0x42, rom[0]);
}

TEST_F(AssembleSourceTest, JumpUsesTheLowNineBits) {
    ASSERT_TRUE(assembleText("org 5ff\ntarget: nop\norg 0\njmp target\n")) << error;
    EXPECT_EQ(0xa00 | 0x1ff, rom[0]);
}

TEST_F(AssembleSourceTest, LabelsAreCaseSensitiveNamesWithUnderscoreAndDigits) {
    ASSERT_TRUE(assembleText("Main_1:\nnop\njmp Main_1\n")) << error;
    EXPECT_EQ(0xa00, rom[1]);
}

TEST_F(AssembleSourceTest, JumpToANumberStillWorks) {
    ASSERT_TRUE(assembleText("jmp 1ff\n")) << error;
    EXPECT_EQ(0xbff, rom[0]);
}

TEST_F(AssembleSourceTest, ErrorNamesTheLine) {
    EXPECT_FALSE(assembleText("nop\nfrobnicate 5\n"));
    EXPECT_NE(std::string::npos, error.find("line 2")) << error;
}

TEST_F(AssembleSourceTest, UnknownLabelIsAnError) {
    EXPECT_FALSE(assembleText("jmp nowhere\n"));
    EXPECT_NE(std::string::npos, error.find("nowhere")) << error;
}

TEST_F(AssembleSourceTest, DoubleLabelIsAnError) {
    EXPECT_FALSE(assembleText("a: nop\na: nop\n"));
    EXPECT_NE(std::string::npos, error.find("line 2")) << error;
}

TEST_F(AssembleSourceTest, CodeBeyondTheRomIsAnError) {
    EXPECT_FALSE(assembleText("org fff\nnop\nnop\n"));
    EXPECT_NE(std::string::npos, error.find("line 3")) << error;
}

TEST_F(AssembleSourceTest, OrgBeyondTheRomIsAnError) {
    EXPECT_FALSE(assembleText("org 1000\n"));
}

TEST_F(AssembleSourceTest, LabelOnlyLineAndInstructionOnSameLine) {
    ASSERT_TRUE(assembleText("a:\nb: nop\njmp a\njmp b\n")) << error;
    EXPECT_EQ(0xa00, rom[1]);
    EXPECT_EQ(0xa00, rom[2]);
}
