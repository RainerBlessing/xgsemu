#include <gtest/gtest.h>
#include <string.h>
#include <string>
#include "../src/filehandler.h"
#include "test_file.h"

class FileHandlerTest : public ::testing::Test {
protected:
    u16 rom[4096];

    void SetUp() override {
        for (int i = 0; i < 4096; i++) {
            rom[i] = 0x5a5;
        }
    }

    bool untouched() const {
        for (int i = 0; i < 4096; i++) {
            if (rom[i] != 0x5a5) {
                return false;
            }
        }
        return true;
    }

    static std::string hexProgram() {
        return hexRecord(0x0000, 0, {0x2a, 0x0c}) + ":00000001FF\n";
    }
};

TEST_F(FileHandlerTest, LoadsAHexFile) {
    TestFile file(".hex", hexProgram());
    ASSERT_TRUE(FileHandler::readFile(file.name(), rom));
    EXPECT_EQ(0x0c2a, rom[0]);
    EXPECT_EQ(0xfff, rom[1]);
}

TEST_F(FileHandlerTest, LoadsAnObjectFile) {
    TestFile file(".obj", std::string("\x2a\x0c", 2));
    ASSERT_TRUE(FileHandler::readFile(file.name(), rom));
    EXPECT_EQ(0x0c2a, rom[0]);
}

TEST_F(FileHandlerTest, TheExtensionIsNotCaseSensitive) {
    TestFile file(".HEX", hexProgram());
    ASSERT_TRUE(FileHandler::readFile(file.name(), rom));
    EXPECT_EQ(0x0c2a, rom[0]);
}

TEST_F(FileHandlerTest, AMissingHexFileIsAnErrorAndLeavesTheRomAlone) {
    EXPECT_FALSE(FileHandler::readFile("no_such_file.hex", rom));
    EXPECT_TRUE(untouched());
}

TEST_F(FileHandlerTest, AMissingObjectFileIsAnErrorAndLeavesTheRomAlone) {
    EXPECT_FALSE(FileHandler::readFile("no_such_file.obj", rom));
    EXPECT_TRUE(untouched());
}

TEST_F(FileHandlerTest, ABrokenHexFileIsAnErrorAndLeavesTheRomAlone) {
    TestFile file(".hex", ":02000000AABB00\n:00000001FF\n");   // wrong checksum
    EXPECT_FALSE(FileHandler::readFile(file.name(), rom));
    EXPECT_TRUE(untouched());
}

TEST_F(FileHandlerTest, AnUnknownExtensionIsAnErrorAndLeavesTheRomAlone) {
    TestFile file(".txt", hexProgram());
    EXPECT_FALSE(FileHandler::readFile(file.name(), rom));
    EXPECT_TRUE(untouched());
}

TEST_F(FileHandlerTest, AFileNameWithoutAnExtensionIsAnError) {
    EXPECT_FALSE(FileHandler::readFile("ab", rom));
    EXPECT_TRUE(untouched());
}

TEST_F(FileHandlerTest, AnExtensionNeedsItsDot) {
    TestFile file("hex", hexProgram());
    EXPECT_FALSE(FileHandler::readFile(file.name(), rom));
    EXPECT_TRUE(untouched());
}

// The fuse words are part of the program file: the FUSEX word has the CF bit that decides
// whether ADD and SUB take the carry flag as input.
TEST_F(FileHandlerTest, ReturnsTheFusexWordOfAHexFile) {
    // FUSE at word 0x1010, FUSEX at word 0x1011: byte address 0x2020
    TestFile file(".hex", hexProgram().substr(0, hexProgram().find(":00000001FF")) +
                              hexRecord(0x2020, 0, {0xaa, 0x0f, 0xbf, 0x0f}) + ":00000001FF\n");
    u16 fusex = 0;
    ASSERT_TRUE(FileHandler::readFile(file.name(), rom, &fusex));
    EXPECT_EQ(0xfbf, fusex);
}

TEST_F(FileHandlerTest, ANonProgrammedFusexWordIsAllOnes) {
    TestFile file(".hex", hexProgram());
    u16 fusex = 0;
    ASSERT_TRUE(FileHandler::readFile(file.name(), rom, &fusex));
    EXPECT_EQ(0xfff, fusex);
}

TEST_F(FileHandlerTest, AnObjectFileHasNoFuses) {
    TestFile file(".obj", std::string("\x2a\x0c", 2));
    u16 fusex = 0;
    ASSERT_TRUE(FileHandler::readFile(file.name(), rom, &fusex));
    EXPECT_EQ(0xfff, fusex);
}

TEST_F(FileHandlerTest, TheFusexWordIsNotTouchedWhenTheFileCannotBeRead) {
    u16 fusex = 0x123;
    EXPECT_FALSE(FileHandler::readFile("no_such_file.hex", rom, &fusex));
    EXPECT_EQ(0x123, fusex);
}
