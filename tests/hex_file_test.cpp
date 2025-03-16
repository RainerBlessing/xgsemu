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
