#include <gtest/gtest.h>
#include <string.h>
#include <string>
#include "../src/objfile.h"
#include "../src/sxkey.h"
#include "test_file.h"

namespace {

// Raw binary image: two bytes per word, low byte first.
std::string image(int words, int firstWord = 0) {
    std::string bytes;
    for (int i = 0; i < words; i++) {
        int word = (firstWord + i) & 0xfff;
        bytes += (char)(word & 0xff);
        bytes += (char)(word >> 8);
    }
    return bytes;
}

}  // namespace

class ObjFileTest : public ::testing::Test {
protected:
    sx_t rom;
    ObjFile objFile;

    void SetUp() override {
        memset(&rom, 0x55, sizeof(rom));
    }
};

TEST_F(ObjFileTest, BytesFormLittleEndianWords) {
    TestFile file(".obj", std::string("\x01\x02\x03\x04", 4));
    ASSERT_TRUE(objFile.readFile(file.name(), &rom));
    EXPECT_EQ(0x0201, rom.rom[0]);
    EXPECT_EQ(0x0403, rom.rom[1]);
}

TEST_F(ObjFileTest, EverythingNotInTheFileIsLeftErased) {
    TestFile file(".obj", std::string("\x01\x02", 2));
    ASSERT_TRUE(objFile.readFile(file.name(), &rom));
    EXPECT_EQ(0xfff, rom.rom[1]);
    EXPECT_EQ(0xfff, rom.rom[SX_MAX_ROM_SIZE - 1]);
    EXPECT_EQ(0xfff, rom.rom[ID_LOC]);
    EXPECT_EQ(0xfff, rom.rom[FUSEX_LOC]);
}

TEST_F(ObjFileTest, AFullImageIsLoadedCompletely) {
    TestFile file(".obj", image(SX_MAX_ROM_SIZE));
    ASSERT_TRUE(objFile.readFile(file.name(), &rom));
    for (int i = 0; i < SX_MAX_ROM_SIZE; i++) {
        ASSERT_EQ(i & 0xfff, rom.rom[i]) << "word " << i;
    }
}

TEST_F(ObjFileTest, ABiggerFileDoesNotWritePastTheProgramMemory) {
    TestFile file(".obj", image(SX_MAX_ROM_SIZE + 100));
    ASSERT_TRUE(objFile.readFile(file.name(), &rom));
    EXPECT_EQ(0xfff, rom.rom[ID_LOC]);          // erased, not the data of the file
    EXPECT_EQ(0x5555, rom.rom[ID_LOC + 20]);    // not touched at all
}

TEST_F(ObjFileTest, AnEmptyFileIsAnEmptyRom) {
    TestFile file(".obj", "");
    EXPECT_TRUE(objFile.readFile(file.name(), &rom));
    EXPECT_EQ(0xfff, rom.rom[0]);
}

TEST_F(ObjFileTest, ANonExistentFileIsAnError) {
    EXPECT_FALSE(objFile.readFile("no_such_file.obj", &rom));
}

TEST_F(ObjFileTest, WhatIsWrittenIsTheRawImage) {
    for (int i = 0; i < SX_MAX_ROM_SIZE; i++) {
        rom.rom[i] = (i * 7 + 3) & 0xfff;
    }
    TestFile file(".obj");
    ASSERT_TRUE(objFile.writeFile(file.name(), &rom));
    std::string contents = file.read();
    ASSERT_EQ((size_t)SX_MAX_ROM_SIZE * 2, contents.size());
    EXPECT_EQ(3, (unsigned char)contents[0]);
    EXPECT_EQ(0, (unsigned char)contents[1]);
    EXPECT_EQ(10, (unsigned char)contents[2]);
}

TEST_F(ObjFileTest, WhatIsWrittenCanBeReadAgain) {
    for (int i = 0; i < SX_MAX_ROM_SIZE; i++) {
        rom.rom[i] = (i * 37 + 5) & 0xfff;
    }
    TestFile file(".obj");
    ASSERT_TRUE(objFile.writeFile(file.name(), &rom));
    sx_t loaded;
    ASSERT_TRUE(objFile.readFile(file.name(), &loaded));
    for (int i = 0; i < SX_MAX_ROM_SIZE; i++) {
        ASSERT_EQ(rom.rom[i], loaded.rom[i]) << "word " << i;
    }
}

TEST_F(ObjFileTest, ADirectoryThatDoesNotExistIsAnError) {
    EXPECT_FALSE(objFile.writeFile("no_such_directory/x.obj", &rom));
}
