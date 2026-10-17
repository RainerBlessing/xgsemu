#include <gtest/gtest.h>
#include <fstream>
#include <string>
#include "../src/hex_file.h"
#include "../src/sxkey.h"

// Fixture für HexFile Tests
class HexFileTest : public ::testing::Test {
protected:
    std::string testFileName;
    sx_t testMemory;
    
    void SetUp() override {
        // Initialisiere den Speicher mit 0xfff (Standard-Reset-Wert)
        for (int i = 0; i < SX_MAX_ROM_SIZE; i++) {
            testMemory.rom[i] = 0xfff;
        }
        
        // Erstelle eine temporäre Test-Datei mit korrektem Checksum
        testFileName = "test_hexfile.hex";
        std::ofstream testFile(testFileName);
        
        // Intel HEX Format mit korrekter Prüfsumme:
        testFile << ":1000000001020304050607080F0E0D0C0B0A090870" << std::endl;

        // 00 Byte Count, 0000 Address, 01 Record Type (EOF), FF Checksum
        testFile << ":00000001FF" << std::endl;
        
        testFile.close();
    }
    
    void TearDown() override {
        // Lösche die temporäre Datei
        std::remove(testFileName.c_str());
    }
};

// Teste das Laden einer HEX-Datei
TEST_F(HexFileTest, TestLoadHexFile) {
    // Lade die Testdatei
    bool result = HexFile::readFile(testFileName.c_str(), &testMemory);
    
    // Überprüfe, ob die Datei geladen wurde
    EXPECT_TRUE(result);
    
    // Überprüfe den Inhalt des Speichers
    // HexFile lädt in Little-Endian Format: niedrigeres Byte in (address), höheres in (address+1)
    // Daher ist ROM[0] = 0x0201 (nicht 0x0102)
    EXPECT_EQ(testMemory.rom[0], 0x0201);
    EXPECT_EQ(testMemory.rom[1], 0x0403);
    EXPECT_EQ(testMemory.rom[2], 0x0605);
    EXPECT_EQ(testMemory.rom[3], 0x0807);
    EXPECT_EQ(testMemory.rom[4], 0x0E0F);
    EXPECT_EQ(testMemory.rom[5], 0x0C0D);
    EXPECT_EQ(testMemory.rom[6], 0x0A0B);
    EXPECT_EQ(testMemory.rom[7], 0x0809);
}

// Teste das Laden einer nicht existierenden Datei
TEST_F(HexFileTest, TestLoadNonExistentFile) {
    // Versuche, eine nicht existierende Datei zu laden
    bool result = HexFile::readFile("non_existent_file.hex", &testMemory);
    
    // Die Funktion sollte false zurückgeben
    EXPECT_FALSE(result);
}

#include <string.h>
#include <vector>
#include "test_file.h"

// A ROM with a few words of guard behind it, to see writes past the end.
struct GuardedRom {
    sx_t sx;
    u16 guard[8];

    GuardedRom() {
        memset(&sx, 0, sizeof(sx));
        for (int i = 0; i < 8; i++) {
            guard[i] = 0x5a5a;
        }
    }

    bool guardIntact() const {
        for (int i = 0; i < 8; i++) {
            if (guard[i] != 0x5a5a) {
                return false;
            }
        }
        return true;
    }
};

static const std::string END = ":00000001FF\n";

class HexFileReadTest : public ::testing::Test {
protected:
    GuardedRom rom;

    bool read(const TestFile& file) {
        return HexFile::readFile(file.name(), &rom.sx);
    }
};

TEST_F(HexFileReadTest, BytesFormLittleEndianWords) {
    TestFile file(".hex", hexRecord(0x0010, 0, {0x34, 0x12, 0x78, 0x06}) + END);
    ASSERT_TRUE(read(file));
    EXPECT_EQ(0x1234, rom.sx.rom[8]);
    EXPECT_EQ(0x0678, rom.sx.rom[9]);
}

TEST_F(HexFileReadTest, AnOddStartAddressFillsTheHighByteOfAWord) {
    TestFile file(".hex", hexRecord(0x0001, 0, {0xab}) + END);
    ASSERT_TRUE(read(file));
    EXPECT_EQ(0xabff, rom.sx.rom[0]);   // the low byte is still the erased 0xff
}

TEST_F(HexFileReadTest, EverythingNotInTheFileIsLeftErased) {
    TestFile file(".hex", hexRecord(0x0000, 0, {0x01, 0x00}) + END);
    ASSERT_TRUE(read(file));
    EXPECT_EQ(0xfff, rom.sx.rom[1]);
    EXPECT_EQ(0xfff, rom.sx.rom[SX_MAX_ROM_SIZE - 1]);
    EXPECT_EQ(0xfff, rom.sx.rom[ID_LOC]);
    EXPECT_EQ(0xfff, rom.sx.rom[FUSE_LOC]);
    EXPECT_EQ(0xfff, rom.sx.rom[FUSEX_LOC]);
}

TEST_F(HexFileReadTest, SeveralRecordsAreCombined) {
    TestFile file(".hex", hexRecord(0x0000, 0, {0x11, 0x01}) + hexRecord(0x0100, 0, {0x22, 0x02}) + END);
    ASSERT_TRUE(read(file));
    EXPECT_EQ(0x0111, rom.sx.rom[0]);
    EXPECT_EQ(0x0222, rom.sx.rom[0x80]);
}

TEST_F(HexFileReadTest, LowerCaseHexDigitsAreAccepted) {
    std::string line = hexRecord(0x0000, 0, {0xab, 0xcd});
    for (size_t i = 1; i < line.size(); i++) {
        line[i] = tolower(line[i]);
    }
    TestFile file(".hex", line + END);
    ASSERT_TRUE(read(file));
    EXPECT_EQ(0xcdab, rom.sx.rom[0]);
}

TEST_F(HexFileReadTest, WindowsLineEndingsAreAccepted) {
    std::string crlf = hexRecord(0x0000, 0, {0xab, 0x01}) + END;
    for (size_t i = crlf.size(); i-- > 0;) {
        if (crlf[i] == '\n') {
            crlf.insert(i, "\r");
        }
    }
    TestFile file(".hex", crlf);
    ASSERT_TRUE(read(file));
    EXPECT_EQ(0x01ab, rom.sx.rom[0]);
}

TEST_F(HexFileReadTest, ALineWithoutAColonIsIgnored) {
    TestFile file(".hex", "this is not a record\n" + hexRecord(0x0000, 0, {0x01, 0x00}) + END);
    ASSERT_TRUE(read(file));
    EXPECT_EQ(0x0001, rom.sx.rom[0]);
}

TEST_F(HexFileReadTest, UnknownRecordTypesAreIgnored) {
    TestFile file(".hex", hexRecord(0x0000, 5, {0x01, 0x02, 0x03, 0x04}) + hexRecord(0x0000, 0, {0x07, 0x00}) + END);
    ASSERT_TRUE(read(file));
    EXPECT_EQ(0x0007, rom.sx.rom[0]);
}

TEST_F(HexFileReadTest, TheExtendedAddressRecordOfGpasmIsIgnored) {
    TestFile file(".hex", hexRecord(0x0000, 4, {0x00, 0x00}) + hexRecord(0x0000, 0, {0x07, 0x00}) + END);
    ASSERT_TRUE(read(file));
    EXPECT_EQ(0x0007, rom.sx.rom[0]);
}

TEST_F(HexFileReadTest, DataAfterTheEndRecordIsNotLoaded) {
    TestFile file(".hex", END + hexRecord(0x0000, 0, {0x07, 0x00}));
    ASSERT_TRUE(read(file));
    EXPECT_EQ(0xfff, rom.sx.rom[0]);
}

TEST_F(HexFileReadTest, AMissingEndRecordIsNotAnError) {
    TestFile file(".hex", hexRecord(0x0000, 0, {0x07, 0x00}));
    EXPECT_TRUE(read(file));
    EXPECT_EQ(0x0007, rom.sx.rom[0]);
}

TEST_F(HexFileReadTest, AnEmptyFileIsAnEmptyRom) {
    TestFile file(".hex", "");
    EXPECT_TRUE(read(file));
    EXPECT_EQ(0xfff, rom.sx.rom[0]);
}

TEST_F(HexFileReadTest, AWrongChecksumIsAnError) {
    std::string line = hexRecord(0x0000, 0, {0x01, 0x02});
    line[line.size() - 2] = line[line.size() - 2] == '0' ? '1' : '0';
    TestFile file(".hex", line + END);
    EXPECT_FALSE(read(file));
}

TEST_F(HexFileReadTest, ANonHexCharacterInTheByteCountIsAnError) {
    TestFile file(".hex", ":ZZ00000000\n" + END);
    EXPECT_FALSE(read(file));
}

TEST_F(HexFileReadTest, ANonHexCharacterInTheAddressIsAnError) {
    TestFile file(".hex", ":0200G0000102FB\n" + END);
    EXPECT_FALSE(read(file));
}

TEST_F(HexFileReadTest, ANonHexCharacterInTheRecordTypeIsAnError) {
    TestFile file(".hex", ":020000XX0102FB\n" + END);
    EXPECT_FALSE(read(file));
}

TEST_F(HexFileReadTest, ANonHexCharacterInTheDataIsAnError) {
    TestFile file(".hex", ":02000000Q102FB\n" + END);
    EXPECT_FALSE(read(file));
}

TEST_F(HexFileReadTest, ATruncatedLineIsAnError) {
    TestFile file(".hex", ":10000000010203\n" + END);
    EXPECT_FALSE(read(file));
}

TEST_F(HexFileReadTest, ALineThatEndsBeforeTheChecksumIsAnError) {
    TestFile file(".hex", ":020000000102\n" + END);
    EXPECT_FALSE(read(file));
}

TEST_F(HexFileReadTest, AnEndRecordWithDataIsAnError) {
    TestFile file(".hex", hexRecord(0x0000, 1, {0x01}));
    EXPECT_FALSE(read(file));
}

TEST_F(HexFileReadTest, TheLastByteOfTheRomImageIsLoaded) {
    unsigned last = ARRAY_SIZE * 2 - 1;
    TestFile file(".hex", hexRecord(last, 0, {0xab}) + END);
    ASSERT_TRUE(read(file));
    EXPECT_EQ(0xab, rom.sx.rom[ARRAY_SIZE - 1] >> 8);
    EXPECT_TRUE(rom.guardIntact());
}

TEST_F(HexFileReadTest, AByteBehindTheRomImageIsAnErrorAndIsNotWritten) {
    unsigned behind = ARRAY_SIZE * 2;
    TestFile file(".hex", hexRecord(behind, 0, {0xab, 0xcd}) + END);
    EXPECT_FALSE(read(file));
    EXPECT_TRUE(rom.guardIntact());
}

TEST_F(HexFileReadTest, ARecordThatReachesBehindTheRomImageIsAnError) {
    unsigned last = ARRAY_SIZE * 2 - 1;
    TestFile file(".hex", hexRecord(last, 0, {0x01, 0x02, 0x03}) + END);
    EXPECT_FALSE(read(file));
    EXPECT_TRUE(rom.guardIntact());
}

class HexFileWriteTest : public ::testing::Test {
protected:
    GuardedRom rom;
    GuardedRom loaded;

    void fill() {
        for (int i = 0; i < SX_MAX_ROM_SIZE; i++) {
            rom.sx.rom[i] = (i * 37 + 5) & 0xfff;
        }
        for (int i = 0; i < SX_ID_SIZE; i++) {
            rom.sx.rom[ID_LOC + i] = 0x100 + i;
        }
        rom.sx.rom[FUSE_LOC] = 0x123;
        rom.sx.rom[FUSEX_LOC] = 0x456;
    }
};

TEST_F(HexFileWriteTest, WhatIsWrittenCanBeReadAgain) {
    fill();
    TestFile file(".hex");
    ASSERT_TRUE(HexFile::writeFile(file.name(), &rom.sx));
    ASSERT_TRUE(HexFile::readFile(file.name(), &loaded.sx));
    for (int i = 0; i < SX_MAX_ROM_SIZE; i++) {
        ASSERT_EQ(rom.sx.rom[i], loaded.sx.rom[i]) << "word " << i;
    }
}

TEST_F(HexFileWriteTest, TheIdAndTheFusesSurviveTheRoundTrip) {
    fill();
    TestFile file(".hex");
    ASSERT_TRUE(HexFile::writeFile(file.name(), &rom.sx));
    ASSERT_TRUE(HexFile::readFile(file.name(), &loaded.sx));
    for (int i = 0; i < SX_ID_SIZE; i++) {
        EXPECT_EQ(rom.sx.rom[ID_LOC + i], loaded.sx.rom[ID_LOC + i]) << "id word " << i;
    }
    EXPECT_EQ(0x123, loaded.sx.rom[FUSE_LOC]);
    EXPECT_EQ(0x456, loaded.sx.rom[FUSEX_LOC]);
}

TEST_F(HexFileWriteTest, TheFileEndsWithTheEndRecord) {
    fill();
    TestFile file(".hex");
    ASSERT_TRUE(HexFile::writeFile(file.name(), &rom.sx));
    std::string contents = file.read();
    ASSERT_GE(contents.size(), END.size());
    EXPECT_EQ(END, contents.substr(contents.size() - END.size()));
}

TEST_F(HexFileWriteTest, EveryRecordHasACorrectChecksum) {
    fill();
    TestFile file(".hex");
    ASSERT_TRUE(HexFile::writeFile(file.name(), &rom.sx));
    // reading checks the checksums, a wrong one would be an error
    EXPECT_TRUE(HexFile::readFile(file.name(), &loaded.sx));
}

TEST_F(HexFileWriteTest, ADirectoryThatDoesNotExistIsAnError) {
    EXPECT_FALSE(HexFile::writeFile("no_such_directory/x.hex", &rom.sx));
}
