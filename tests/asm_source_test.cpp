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
    ASSERT_TRUE(assembleText("org 242\nsub1: ret\norg 0\ncall sub1\n")) << error;
    EXPECT_EQ(0x900 | 0x42, rom[0]);
}

TEST_F(AssembleSourceTest, CallToTheSecondHalfOfAPageIsAnError) {
    EXPECT_FALSE(assembleText("org 142\nsub1: ret\norg 0\ncall sub1\n"));
    EXPECT_NE(std::string::npos, error.find("line 4")) << error;
    EXPECT_NE(std::string::npos, error.find("sub1")) << error;
}

TEST_F(AssembleSourceTest, CallToTheLastWordOfTheFirstHalfWorks) {
    ASSERT_TRUE(assembleText("org ff\nsub1: ret\norg 0\ncall sub1\n")) << error;
    EXPECT_EQ(0x900 | 0xff, rom[0]);
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

TEST_F(AssembleSourceTest, EquDefinesARegisterName) {
    ASSERT_TRUE(assembleText("counter equ 10\nmov counter, w\nmov w, counter\n")) << error;
    EXPECT_EQ(0x020 | 0x10, rom[0]);
    EXPECT_EQ(0x200 | 0x10, rom[1]);
}

TEST_F(AssembleSourceTest, EquDefinesALiteral) {
    ASSERT_TRUE(assembleText("limit equ 2a\nmov w, #limit\n")) << error;
    EXPECT_EQ(0xc2a, rom[0]);
}

TEST_F(AssembleSourceTest, EquDoesNotUseUpAnAddress) {
    ASSERT_TRUE(assembleText("x equ 5\nnop\n")) << error;
    EXPECT_EQ(0x000, rom[0]);
    EXPECT_EQ(UNUSED, rom[1]);
}

TEST_F(AssembleSourceTest, EquCanUseAnEarlierName) {
    ASSERT_TRUE(assembleText("a equ 8\nb equ a\nmov b, w\n")) << error;
    EXPECT_EQ(0x028, rom[0]);
}

TEST_F(AssembleSourceTest, NameWorksWithABit) {
    ASSERT_TRUE(assembleText("flags equ 12\nsetb flags.3\n")) << error;
    EXPECT_EQ(0x500 | (3 << 5) | 0x12, rom[0]);
}

TEST_F(AssembleSourceTest, SxRegistersHaveNames) {
    ASSERT_TRUE(assembleText("mov w, status\nsetb status.2\nmov w, rtcc\nmov fsr, w\nmov w, rb\n")) << error;
    EXPECT_EQ(0x200 | 3, rom[0]);
    EXPECT_EQ(0x500 | (2 << 5) | 3, rom[1]);
    EXPECT_EQ(0x200 | 1, rom[2]);
    EXPECT_EQ(0x020 | 4, rom[3]);
    EXPECT_EQ(0x200 | 6, rom[4]);
}

TEST_F(AssembleSourceTest, PortControlKeepsItsName) {
    ASSERT_TRUE(assembleText("mov !ra, w\n")) << error;
    EXPECT_EQ(0x005, rom[0]);
}

TEST_F(AssembleSourceTest, RegisterNameCannotBeDefinedAgain) {
    EXPECT_FALSE(assembleText("ra: nop\n"));
    EXPECT_NE(std::string::npos, error.find("line 1")) << error;
}

TEST_F(AssembleSourceTest, DwStoresRawWords) {
    ASSERT_TRUE(assembleText("dw 123, abc\ndw 0\n")) << error;
    EXPECT_EQ(0x123, rom[0]);
    EXPECT_EQ(0xabc, rom[1]);
    EXPECT_EQ(0x000, rom[2]);
}

TEST_F(AssembleSourceTest, DwTakesLabelsAndNamesAndKeepsTheFullAddress) {
    ASSERT_TRUE(assembleText("k equ 7\norg 5ff\ntable: dw k, table\n")) << error;
    EXPECT_EQ(0x007, rom[0x5ff]);
    EXPECT_EQ(0x5ff, rom[0x600]);
}

TEST_F(AssembleSourceTest, LabelBehindDwCountsEveryWord) {
    ASSERT_TRUE(assembleText("dw 1, 2, 3\nend: jmp end\n")) << error;
    EXPECT_EQ(0xa00 | 3, rom[3]);
}

TEST_F(AssembleSourceTest, DwValueTooBigIsAnError) {
    EXPECT_FALSE(assembleText("dw 1000\n"));
    EXPECT_NE(std::string::npos, error.find("line 1")) << error;
}

TEST_F(AssembleSourceTest, DwWithoutValueIsAnError) {
    EXPECT_FALSE(assembleText("dw\n"));
}

TEST_F(AssembleSourceTest, EquTwiceIsAnError) {
    EXPECT_FALSE(assembleText("x equ 1\nx equ 2\n"));
    EXPECT_NE(std::string::npos, error.find("line 2")) << error;
}

TEST_F(AssembleSourceTest, EquWithUnknownNameIsAnError) {
    EXPECT_FALSE(assembleText("x equ y\n"));
}

TEST_F(AssembleSourceTest, ReservedWordsCannotBeNames) {
    EXPECT_FALSE(assembleText("w equ 5\n"));
    EXPECT_FALSE(assembleText("m: nop\n"));
}

TEST_F(AssembleSourceTest, EquMayBeUsedBeforeItsDefinition) {
    ASSERT_TRUE(assembleText("mov late, w\nlate equ 5\n")) << error;
    EXPECT_EQ(0x025, rom[0]);
}

// automatic page instructions

TEST_F(AssembleSourceTest, AutopageInsertsPageBeforeAJumpToAnotherPage) {
    ASSERT_TRUE(assembleText("autopage on\njmp far\norg 200\nfar: nop\n")) << error;
    EXPECT_EQ(0x011, rom[0]);
    EXPECT_EQ(0xa00 | 0, rom[1]);
    EXPECT_EQ(0x000, rom[0x200]);
}

TEST_F(AssembleSourceTest, AutopageLeavesAJumpWithinThePageAlone) {
    ASSERT_TRUE(assembleText("autopage on\njmp near\nnear: nop\n")) << error;
    EXPECT_EQ(0xa00 | 1, rom[0]);
    EXPECT_EQ(0x000, rom[1]);
}

TEST_F(AssembleSourceTest, WithoutAutopageNothingIsInserted) {
    ASSERT_TRUE(assembleText("jmp far\norg 200\nfar: nop\n")) << error;
    EXPECT_EQ(0xa00, rom[0]);
    EXPECT_EQ(UNUSED, rom[1]);
}

TEST_F(AssembleSourceTest, AutopageCanBeSwitchedOff) {
    ASSERT_TRUE(assembleText("autopage on\nautopage off\njmp far\norg 200\nfar: nop\n")) << error;
    EXPECT_EQ(0xa00, rom[0]);
}

TEST_F(AssembleSourceTest, AutopageAlsoPrecedesACall) {
    ASSERT_TRUE(assembleText("autopage on\ncall sub\norg 200\nsub: retp\n")) << error;
    EXPECT_EQ(0x011, rom[0]);
    EXPECT_EQ(0x900 | 0, rom[1]);
}

TEST_F(AssembleSourceTest, AutopageLeavesNumbersAlone) {
    ASSERT_TRUE(assembleText("autopage on\njmp 1ff\n")) << error;
    EXPECT_EQ(0xbff, rom[0]);
    EXPECT_EQ(UNUSED, rom[1]);
}

TEST_F(AssembleSourceTest, AutopageMovesTheLabelsBehindIt) {
    ASSERT_TRUE(assembleText("autopage on\njmp far\nnext: nop\norg 200\nfar: jmp next\n")) << error;
    EXPECT_EQ(0x011, rom[0]);
    EXPECT_EQ(0x000, rom[2]);
    EXPECT_EQ(0x010, rom[0x200]);
    EXPECT_EQ(0xa00 | 2, rom[0x201]);
}

TEST_F(AssembleSourceTest, LabelOnTheJumpPointsAtTheInsertedPage) {
    ASSERT_TRUE(assembleText("autopage on\nstart: jmp far\norg 200\nfar: jmp start\n")) << error;
    EXPECT_EQ(0x011, rom[0]);
    EXPECT_EQ(0x010, rom[0x200]);
    EXPECT_EQ(0xa00 | 0, rom[0x201]);
}

TEST_F(AssembleSourceTest, AutopageDoesNotRepeatAnExplicitPage) {
    ASSERT_TRUE(assembleText("autopage on\npage 1\njmp far\norg 200\nfar: nop\n")) << error;
    EXPECT_EQ(0x011, rom[0]);
    EXPECT_EQ(0xa00, rom[1]);
    EXPECT_EQ(UNUSED, rom[2]);
}

TEST_F(AssembleSourceTest, AutopageRepeatsUntilTheAddressesAreStable) {
    // The first page moves the label lab across the page boundary, which
    // makes the second jump need a page as well.
    std::string source = "autopage on\njmp far\n";
    for (int i = 0; i < 0x1fd; i++) {
        source += "dw 0\n";
    }
    source += "jmp lab\nlab: nop\norg 400\nfar: nop\n";
    ASSERT_TRUE(assembleText(source)) << error;
    EXPECT_EQ(0x012, rom[0]);
    EXPECT_EQ(0x011, rom[0x1ff]);
    EXPECT_EQ(0xa00 | 1, rom[0x200]);
    EXPECT_EQ(0x000, rom[0x201]);
}

TEST_F(AssembleSourceTest, AutopageNeedsOnOrOff) {
    EXPECT_FALSE(assembleText("autopage\n"));
    EXPECT_FALSE(assembleText("autopage maybe\n"));
    EXPECT_NE(std::string::npos, error.find("line 1")) << error;
}

// firsthalf: a place for the target of a call

TEST_F(AssembleSourceTest, FirsthalfLeavesTheFirstHalfOfAPageAlone) {
    ASSERT_TRUE(assembleText("nop\nfirsthalf\nhere: nop\n")) << error;
    EXPECT_EQ(0x000, rom[1]);
}

TEST_F(AssembleSourceTest, FirsthalfMovesOnToTheNextPageFromTheSecondHalf) {
    ASSERT_TRUE(assembleText("org 100\nfirsthalf\nnop\n")) << error;
    EXPECT_EQ(UNUSED, rom[0x100]);
    EXPECT_EQ(0x000, rom[0x200]);
}

TEST_F(AssembleSourceTest, FirsthalfAtTheEndOfAPage) {
    ASSERT_TRUE(assembleText("org 1ff\nfirsthalf\nnop\n")) << error;
    EXPECT_EQ(0x000, rom[0x200]);
}

TEST_F(AssembleSourceTest, FirsthalfBindsALabelOnItsLine) {
    // call needs a target in the first half: the label has to move with the code
    ASSERT_TRUE(assembleText("org 150\nfirsthalf\nsub: ret\norg 0\ncall sub\n")) << error;
    EXPECT_EQ(0x900 | 0x00, rom[0]);
    EXPECT_EQ(0x00c, rom[0x200]);
}

TEST_F(AssembleSourceTest, FirsthalfBeyondTheRomIsAnError) {
    EXPECT_FALSE(assembleText("org fff\nfirsthalf\nnop\n"));
}

// autopage keeps the page right when the code runs across a page boundary

TEST_F(AssembleSourceTest, AutopageSetsThePageWhereCodeRunsIntoTheNextOne) {
    std::string source = "autopage on\n";
    for (int i = 0; i < 0x200; i++) {
        source += "nop\n";
    }
    source += "inc 10\n";
    ASSERT_TRUE(assembleText(source)) << error;
    EXPECT_EQ(0x000, rom[0x1ff]);
    EXPECT_EQ(0x011, rom[0x200]);
    EXPECT_EQ(0x2b0, rom[0x201]);
}

TEST_F(AssembleSourceTest, NoPageAtTheBoundaryWithoutAutopage) {
    std::string source;
    for (int i = 0; i < 0x201; i++) {
        source += "nop\n";
    }
    ASSERT_TRUE(assembleText(source)) << error;
    EXPECT_EQ(0x000, rom[0x200]);
}

TEST_F(AssembleSourceTest, NoPageAtABoundaryReachedByOrg) {
    ASSERT_TRUE(assembleText("autopage on\norg 200\nnop\n")) << error;
    EXPECT_EQ(0x000, rom[0x200]);
}

TEST_F(AssembleSourceTest, LabelAtTheBoundaryPointsAtThePageInstruction) {
    std::string source = "autopage on\n";
    for (int i = 0; i < 0x200; i++) {
        source += "nop\n";
    }
    source += "next: nop\njmp next\n";
    ASSERT_TRUE(assembleText(source)) << error;
    EXPECT_EQ(0x011, rom[0x200]);
    EXPECT_EQ(0xa00 | 0, rom[0x202]);       // next is at 0x200
}

TEST_F(AssembleSourceTest, BoundaryPageAfterData) {
    std::string source = "autopage on\n";
    for (int i = 0; i < 0x1ff; i++) {
        source += "dw 123\n";
    }
    source += "nop\nnop\n";
    ASSERT_TRUE(assembleText(source)) << error;
    EXPECT_EQ(0x000, rom[0x1ff]);
    EXPECT_EQ(0x011, rom[0x200]);
    EXPECT_EQ(0x000, rom[0x201]);
}

// two pieces of code must not share a word

TEST_F(AssembleSourceTest, OverlappingCodeIsAnError) {
    EXPECT_FALSE(assembleText("org 10\nnop\nnop\norg 11\nnop\n"));
    EXPECT_NE(std::string::npos, error.find("line 5")) << error;
    EXPECT_NE(std::string::npos, error.find("11")) << error;
}

TEST_F(AssembleSourceTest, DataOverlappingCodeIsAnError) {
    EXPECT_FALSE(assembleText("org 10\nnop\norg 10\ndw 1\n"));
}

TEST_F(AssembleSourceTest, CodeRunningIntoTheResetVectorIsAnError) {
    EXPECT_FALSE(assembleText("org fff\njmp 0\norg ffe\nnop\nnop\n"));
}

TEST_F(AssembleSourceTest, NeighbouringCodeIsFine) {
    ASSERT_TRUE(assembleText("org 10\nnop\norg 11\nnop\n")) << error;
}

TEST_F(AssembleSourceTest, IndfIsRegisterZero) {
    ASSERT_TRUE(assembleText("mov w, indf\nmov indf, w\ninc indf\nsnb indf.3\n")) << error;
    EXPECT_EQ(0x200, rom[0]);
    EXPECT_EQ(0x020, rom[1]);
    EXPECT_EQ(0x2a0, rom[2]);
    EXPECT_EQ(0x660, rom[3]);
}

// the addresses of the labels

TEST_F(AssembleSourceTest, LabelAddressesCanBeAskedFor) {
    std::map<std::string, int> labels;
    ASSERT_TRUE(assembleSource("start: nop\nloop: nop\njmp loop\norg 100\nlater: nop\nsize equ 5\n", rom, ROM_SIZE, &error, &labels)) << error;
    EXPECT_EQ(0, labels.at("start"));
    EXPECT_EQ(1, labels.at("loop"));
    EXPECT_EQ(0x100, labels.at("later"));
}

TEST_F(AssembleSourceTest, NamesFromEquAreNoLabels) {
    std::map<std::string, int> labels;
    ASSERT_TRUE(assembleSource("size equ 5\nnop\n", rom, ROM_SIZE, &error, &labels)) << error;
    EXPECT_EQ(0u, labels.count("size"));
    EXPECT_EQ(0u, labels.count("rtcc"));
}

TEST_F(AssembleSourceTest, LabelAddressesIncludeTheInsertedPages) {
    std::map<std::string, int> labels;
    ASSERT_TRUE(assembleSource("autopage on\nstart: jmp far\nnext: nop\norg 200\nfar: nop\n", rom, ROM_SIZE, &error, &labels)) << error;
    EXPECT_EQ(0, labels.at("start"));
    EXPECT_EQ(2, labels.at("next"));        // after the page and the jump
    EXPECT_EQ(0x200, labels.at("far"));
}

TEST_F(AssembleSourceTest, NoLabelsAreGivenOnAnError) {
    std::map<std::string, int> labels;
    EXPECT_FALSE(assembleSource("a: nop\nfrobnicate\n", rom, ROM_SIZE, &error, &labels));
    EXPECT_TRUE(labels.empty());
}
