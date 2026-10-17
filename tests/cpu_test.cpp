#include <gtest/gtest.h>
#include <vector>
#include "cpu.h"
#include "../src/Video.h"

// Einfache Mock-Klasse für Video
class MockVideo : public Video {
public:
    MockVideo() : Video() {}
    
    // Die Methode muss genau dem virtuellen Funktionsprototyp in Video entsprechen
    void changeVideo(int cycle, unsigned char val) override {
        lastCycle = cycle;
        lastValue = val;
        calls++;
    }
    
    int calls = 0;
    int lastCycle = 0;
    unsigned char lastValue = 0;
};

// Fixture für CPU-Tests
class CpuTest : public ::testing::Test {
protected:
    MockVideo* videoMock;
    Cpu* cpu;
    
    void SetUp() override {
        videoMock = new MockVideo();
        cpu = new Cpu(videoMock);
    }
    
    void TearDown() override {
        delete cpu;
        delete videoMock;
    }
    
    // Hilfsfunktion zum Setzen eines Wertes im ROM
    void setRomValue(USHORT address, USHORT value) {
        USHORT* rom = cpu->getROM();
        rom[address] = value;
    }
    
    // Hilfsfunktion zum Setzen eines Wertes im RAM
    void setRamValue(USHORT address, unsigned char value) {
        unsigned char* ram = cpu->getRAM();
        ram[address] = value;
    }
};

// Teste die CPU.reset() Funktion
TEST_F(CpuTest, ResetClearsMemoryAndSetsPC) {
    // Schreibe einige Werte in RAM und ROM
    unsigned char* ram = cpu->getRAM();
    USHORT* rom = cpu->getROM();
    USHORT* pc = cpu->getPC();
    
    ram[0] = 0x42;
    rom[0] = 0x1234;
    *pc = 0x123;  // Setze PC auf einen anderen Wert als 0xfff
    
    // Führe reset aus
    cpu->reset();
    
    // Überprüfe, ob RAM zurückgesetzt wurde
    EXPECT_EQ(ram[0], 0);
    
    // Überprüfe, ob PC auf 0xfff gesetzt wurde
    EXPECT_EQ(*pc, 0xfff);
}

// Teste grundlegende Opcode-Ausführung: NOP
TEST_F(CpuTest, ExecuteNopInstruction) {
    // Setze PC auf 0
    USHORT* pc = cpu->getPC();
    *pc = 0;
    
    // NOP bei Adresse 0
    setRomValue(0, 0x0000);
    
    // Führe eine Instruktion aus
    cpu->execute();
    
    // PC sollte jetzt 1 sein nach dem Ausführen von NOP
    EXPECT_EQ(*pc, 1);
}

// Teste MOV W,#lit Instruction (1100 kkkk kkkk, die Befehle des SX52 sind 12 Bit breit)
TEST_F(CpuTest, ExecuteMovWLitInstruction) {
    // Setze PC auf 0
    USHORT* pc = cpu->getPC();
    *pc = 0;
    
    // MOV W,#42 - Lade die Konstante 42 in W
    setRomValue(0, 0xC2A);
    
    unsigned short* rom = cpu->getROM();
    // Führe die Instruktion aus
    cpu->execute();
    
    // W-Register sollte jetzt 42 sein (W ist RAM[261])
    unsigned char* ram = cpu->getRAM();
    EXPECT_EQ(ram[261], 42);
    
    // PC sollte inkrementiert worden sein
    EXPECT_EQ(*pc, 1);
}

// Test für MOV W,#lit Instruction mit explizitem Debugging des Opcode-Extraktions
TEST_F(CpuTest, ExecuteMovWLitInstructionWithOpcodeCheck) {
    // Setze PC auf 0
    USHORT* pc = cpu->getPC();
    *pc = 0;
    
    // MOV W,#42 - Lade die Konstante 42 in W
    // 0xC2A entspricht dem Opcode 1100 0010 1010
    // Die höchsten 4 Bits der 12 Bit (1100) = 0xC
    USHORT instruction = 0xC2A;
    setRomValue(0, instruction);
    
    // Führe eine Manuellen Test des Opcode-Extraktionsprozesses durch
    USHORT mne = instruction;
    USHORT opcode = (mne >> 8) & 0xF;
    
    // Überprüfe, ob der extrahierte Opcode dem erwarteten Wert entspricht
    EXPECT_EQ(opcode, 0xC) << "Der extrahierte Opcode sollte 0xC sein, ist aber " << opcode;
    
    // Führe die Instruktion aus
    cpu->execute();
    
    // W-Register sollte jetzt 42 sein (W ist RAM[261])
    unsigned char* ram = cpu->getRAM();
    EXPECT_EQ(ram[261], 42) << "W-Register (RAM[261]) sollte 42 sein, ist aber " << (int)ram[261];
    
    // PC sollte inkrementiert worden sein
    EXPECT_EQ(*pc, 1) << "PC sollte auf 1 inkrementiert worden sein, ist aber " << *pc;
}

// Teste CLR W Instruction (0x0040)
TEST_F(CpuTest, ExecuteClrWInstruction) {
    // Setze PC auf 0
    USHORT* pc = cpu->getPC();
    *pc = 0;
    
    // Setze W-Register auf einen nicht-Null-Wert
    unsigned char* ram = cpu->getRAM();
    ram[261] = 0x42;
    
    // CLR W - Setze W auf 0
    setRomValue(0, 0x0040);
    
    // Führe eine Instruktion aus
    cpu->execute();
    
    // W-Register sollte jetzt 0 sein
    EXPECT_EQ(ram[261], 0);
    
    // Zero-Flag sollte gesetzt sein (im Status-Register, Bit 2)
    EXPECT_TRUE(ram[3] & (1 << 2));
}

// Teste INC fr Instruction (für Register 10 - kann ein beliebiges Register sein)
TEST_F(CpuTest, ExecuteIncFrInstruction) {
    // Setze PC auf 0
    USHORT* pc = cpu->getPC();
    *pc = 0;
    
    // Setze RAM[10] auf 41
    setRamValue(10, 41);
    
    // INC 10 - Inkrementiere Register 10 (Opcode 0x02AA)
    // 0x02AA entspricht dem Opcode Pattern 0010 101f ffff mit f=10 (0x0A)
    setRomValue(0, 0x02AA);
    
    // Führe eine Instruktion aus
    cpu->execute();
    
    // RAM[10] sollte jetzt 42 sein
    unsigned char* ram = cpu->getRAM();
    EXPECT_EQ(ram[10], 42);
    
    // PC sollte inkrementiert worden sein
    EXPECT_EQ(*pc, 1);
}

// Teste Joystick-Funktionalität
TEST_F(CpuTest, JoystickRegisterAccessible) {
    // Hole Joystick-Register
    Uint16* joystickReg = cpu->getJoystrickr();
    
    // Setze einen Wert
    *joystickReg = 0x1234;
    
    // Überprüfe, ob der Wert gesetzt wurde
    EXPECT_EQ(*joystickReg, 0x1234);
}

// Teste, dass das Video-Objekt verwendet wird, wenn ein bestimmter RAM-Bereich geändert wird
TEST_F(CpuTest, VideoObjectUsed) {
    // Setze PC auf 0
    USHORT* pc = cpu->getPC();
    *pc = 0;
    
    // Setze W-Register auf 0x42
    unsigned char* ram = cpu->getRAM();
    ram[261] = 0x42;
    
    // MOV 9,W - Bewege W in Register 9 (RE)
    // Opcode 0x0029 entspricht 0000 001f ffff mit f=9
    setRomValue(0, 0x0029);
    
    // Führe eine Instruktion aus
    cpu->execute();
    
    // Prüfe, ob Register 9 jetzt den Wert 0x42 hat
    EXPECT_EQ(ram[9], 0x42);
    
    // Das Video-Objekt sollte mit dem Wert 0x42 aufgerufen worden sein
    // Hinweis: Dies funktioniert nur, wenn die CPU die changeVideo-Methode 
    // für Register 9 (RE) aufruft, was in der aktuellen CPU-Implementierung
    // möglicherweise nicht der Fall ist.
    EXPECT_EQ(videoMock->lastValue, 0x42);
}


// "jmp PC+W" is "add PC,W" and clears the carry flag when the addition stays
// inside the low byte of the program counter. Rem-Pac relies on that before
// it shifts sprite bits with rr/rl.
TEST_F(CpuTest, JumpPcPlusWClearsCarryAboveFirstPage) {
    USHORT* pc = cpu->getPC();
    unsigned char* ram = cpu->getRAM();
    *pc = 0xa00;
    ram[261] = 0x03;         // W
    ram[3] |= 1;             // carry set
    setRomValue(0xa00, 0x1e2);   // add PC,W (jmp PC+W)

    cpu->execute();

    EXPECT_EQ(0, ram[3] & 1);
}

TEST_F(CpuTest, JumpPcPlusWJumpsBehindTheNextInstruction) {
    USHORT* pc = cpu->getPC();
    unsigned char* ram = cpu->getRAM();
    *pc = 0xa00;
    ram[261] = 0x03;
    setRomValue(0xa00, 0x1e2);

    cpu->execute();

    EXPECT_EQ(0xa04, *pc);
}

// The page bits of the status register (7:5) are the upper bits of the return
// address. The lower bits of the return address must not end up in the flags.
TEST_F(CpuTest, RetpKeepsCarryZeroAndDigitCarryFlags) {
    USHORT* pc = cpu->getPC();
    unsigned char* ram = cpu->getRAM();
    *pc = 0x137;                  // the return address 0x138 has bits 4 and 5 set
    setRomValue(0x137, 0x920);    // call 0x20 (page 0)
    setRomValue(0x20, 0x00d);     // retp
    ram[3] = 0;

    cpu->execute();               // call
    cpu->execute();               // retp

    EXPECT_EQ(0x138, *pc);
    EXPECT_EQ(0, ram[3] & 0x07);
}

TEST_F(CpuTest, RetpRestoresThePageOfTheReturnAddress) {
    USHORT* pc = cpu->getPC();
    unsigned char* ram = cpu->getRAM();
    *pc = 0x5f0;
    ram[3] = 0x00;
    setRomValue(0x5f0, 0x920);    // call 0x20 in page 0
    setRomValue(0x20, 0x00d);     // retp

    cpu->execute();
    cpu->execute();

    EXPECT_EQ(0x5f1, *pc);
    EXPECT_EQ(2 << 5, ram[3] & 0xe0);   // page 2 (0x400..0x5ff)
}

// Cycle counts from the SX52 instruction set. Rem-Pac pads its timing with
// iread (4 cycles each), so a wrong count shifts everything on the scanline.
class CpuCycleTest : public CpuTest {
protected:
    int cyclesOf(USHORT opcode, USHORT address = 0x100) {
        *cpu->getPC() = address;
        setRomValue(address, opcode);
        int before = cpu->getCycle();
        cpu->execute();
        return cpu->getCycle() - before;
    }
};

TEST_F(CpuCycleTest, IreadTakesFourCycles) {
    EXPECT_EQ(4, cyclesOf(0x041));
}

TEST_F(CpuCycleTest, JumpPcPlusWTakesThreeCycles) {
    cpu->getRAM()[261] = 0;
    EXPECT_EQ(3, cyclesOf(0x1e2));
}

TEST_F(CpuCycleTest, SetbOnPortTakesOneCycle) {
    EXPECT_EQ(1, cyclesOf(0x5a5));   // setb RA.5 (bit 5, register 5)
}

TEST_F(CpuCycleTest, ClrbOnPortTakesOneCycle) {
    EXPECT_EQ(1, cyclesOf(0x4a5));   // clrb RA.5
}

TEST_F(CpuCycleTest, JumpTakesThreeCycles) {
    EXPECT_EQ(3, cyclesOf(0xa00 | 0x10));
}

// Cycle counts of the instruction families, from the SX52 instruction set
// table in the user guide (appendix A). Skipping instructions take one more
// cycle when they skip.
TEST_F(CpuCycleTest, InstructionFamiliesMatchTheInstructionSetTable) {
    const int F = 0x0c;  // a plain register
    struct Case {
        const char* name;
        USHORT opcode;
        unsigned char registerValue;
        int cycles;
    };
    const Case cases[] = {
        {"nop", 0x000, 0, 1},
        {"mov !option,w", 0x002, 0, 1},
        {"clr !wdt", 0x004, 0, 1},
        {"mov !ra,w", 0x005, 0, 1},
        {"page", 0x011, 0, 1},
        {"bank", 0x019, 0, 1},
        {"clr w", 0x040, 0, 1},
        {"mov w,m", 0x042, 0, 1},
        {"mov m,w", 0x043, 0, 1},
        {"mode #lit", 0x055, 0, 1},
        {"mov fr,w", 0x020 | F, 0, 1},
        {"clr fr", 0x060 | F, 0, 1},
        {"sub fr,w", 0x0a0 | F, 0, 1},
        {"mov w,fr-w", 0x080 | F, 0, 1},
        {"dec fr", 0x0e0 | F, 0, 1},
        {"inc fr", 0x2a0 | F, 0, 1},
        {"or w,fr", 0x100 | F, 0, 1},
        {"and fr,w", 0x160 | F, 0, 1},
        {"xor fr,w", 0x1a0 | F, 0, 1},
        {"add w,fr", 0x1c0 | F, 0, 1},
        {"add fr,w", 0x1e0 | F, 0, 1},
        {"mov w,fr", 0x200 | F, 0, 1},
        {"test fr", 0x220 | F, 0, 1},
        {"not fr", 0x260 | F, 0, 1},
        {"rr fr", 0x320 | F, 0, 1},
        {"rl fr", 0x360 | F, 0, 1},
        {"swap fr", 0x3a0 | F, 0, 1},
        {"mov w,#lit", 0xc05, 0, 1},
        {"and w,#lit", 0xe05, 0, 1},
        {"setb fr.1", 0x520 | F, 0, 1},
        {"clrb fr.1", 0x420 | F, 0, 1},
        {"decsz without skip", 0x2e0 | F, 5, 1},
        {"decsz with skip", 0x2e0 | F, 1, 2},
        {"incsz without skip", 0x3e0 | F, 5, 1},
        {"incsz with skip", 0x3e0 | F, 0xff, 2},
        {"movsz w,--fr without skip", 0x2c0 | F, 5, 1},
        {"movsz w,--fr with skip", 0x2c0 | F, 1, 2},
        {"movsz w,++fr without skip", 0x3c0 | F, 5, 1},
        {"movsz w,++fr with skip", 0x3c0 | F, 0xff, 2},
        {"sb without skip", 0x720 | F, 0x00, 1},
        {"sb with skip", 0x720 | F, 0x02, 2},
        {"snb without skip", 0x620 | F, 0x02, 1},
        {"snb with skip", 0x620 | F, 0x00, 2},
        {"call", 0x910, 0, 3},
    };
    for (const Case& c : cases) {
        SCOPED_TRACE(c.name);
        cpu->getRAM()[F] = c.registerValue;
        EXPECT_EQ(c.cycles, cyclesOf(c.opcode));
    }
}

// The speed of the emulated program is given by the clock cycles, not by the
// number of instructions: jmp and call take 3 cycles, iread 4.
TEST_F(CpuCycleTest, RunCyclesStopsAfterTheRequestedCycles) {
    // the ROM is filled with nops by reset, one cycle each
    cpu->runCycles(1000);
    EXPECT_EQ(1000, cpu->getCycle());
}

TEST_F(CpuCycleTest, RunCyclesCountsCyclesAndNotInstructions) {
    // a loop of one jmp (3 cycles), 300 cycles are 100 instructions
    *cpu->getPC() = 0x100;
    setRomValue(0x100, 0xa00 | 0x100);   // jmp 0x100
    cpu->runCycles(300);
    EXPECT_EQ(300, cpu->getCycle());
}

TEST_F(CpuCycleTest, RunCyclesMayOvershootByTheLastInstruction) {
    *cpu->getPC() = 0x100;
    setRomValue(0x100, 0xa00 | 0x100);   // jmp 0x100, 3 cycles
    cpu->runCycles(301);
    EXPECT_EQ(303, cpu->getCycle());
}

TEST_F(CpuCycleTest, RunCyclesCarriesTheOvershootIntoTheNextCall) {
    *cpu->getPC() = 0x100;
    setRomValue(0x100, 0xa00 | 0x100);   // jmp 0x100, 3 cycles
    cpu->runCycles(301);                 // ends at 303, 2 cycles too far
    cpu->runCycles(301);                 // target 602, reached in steps of 3: 603
    // without the carried surplus the target would be 604 and the end 606
    EXPECT_EQ(603, cpu->getCycle());
}

// clr fr on a port register has the same side effects as mov fr,w: clearing RE
// puts black on the video, clearing RA runs the joystick code.
TEST_F(CpuTest, ClrReWritesBlackToVideo) {
    *cpu->getPC() = 0;
    cpu->getRAM()[9] = 0xff;
    setRomValue(0, 0x069);   // clr RE

    cpu->execute();

    EXPECT_EQ(0, cpu->getRAM()[9]);
    EXPECT_EQ(1, videoMock->calls);
    EXPECT_EQ(0, videoMock->lastValue);
}

TEST_F(CpuTest, ClrRaDoesNotWriteToVideo) {
    *cpu->getPC() = 0;
    cpu->getRAM()[5] = 0xff;
    setRomValue(0, 0x065);   // clr RA

    cpu->execute();

    EXPECT_EQ(0, cpu->getRAM()[5]);
    EXPECT_EQ(0, videoMock->calls);
}

TEST_F(CpuTest, ClrNormalRegisterDoesNotWriteToVideo) {
    *cpu->getPC() = 0;
    cpu->getRAM()[0x0c] = 0xff;
    setRomValue(0, 0x06c);   // clr 0x0c

    cpu->execute();

    EXPECT_EQ(0, cpu->getRAM()[0x0c]);
    EXPECT_EQ(0, videoMock->calls);
}

TEST_F(CpuTest, ClrSetsTheZeroFlag) {
    *cpu->getPC() = 0;
    cpu->getRAM()[3] = 0;
    setRomValue(0, 0x06c);   // clr 0x0c

    cpu->execute();

    EXPECT_EQ(1 << 2, cpu->getRAM()[3] & (1 << 2));
}

// The external SRAM is addressed through port C and D: bit 4 of RC clocks one
// bit (bit 5) into a 12 bit page register, bit 6 is the chip enable.
class CpuSramTest : public CpuTest {
protected:
    void run(USHORT opcode) {
        *cpu->getPC() = 0x100;
        setRomValue(0x100, opcode);
        cpu->execute();
    }

    void configureAllPortsAsOutputs() {
        cpu->getRAM()[261] = 0;
        run(0x05f);   // mode #0xf
        run(0x007);   // mov !rc,w
    }

    void clockPageBit(int bit) {
        unsigned char* rc = &cpu->getRAM()[7];
        *rc = (*rc & ~0x20) | (bit << 5);
        run(0x587);   // setb RC.4
        run(0x487);   // clrb RC.4
    }
};

// Characterisation: only the low 12 bits of the page take part in the address.
// (Address bit 16, RA7, is lost because the address is a 16 bit variable.)
TEST_F(CpuSramTest, OnlyTheLast12ClockedBitsFormThePage) {
    configureAllPortsAsOutputs();
    unsigned char* ram = cpu->getRAM();
    ram[7] = 1 << 6;          // chip disabled
    for (int i = 0; i < 20; i++) {
        clockPageBit(1);      // more than 12 bits: the oldest ones fall out
    }
    ram[8] = 0x5a;            // RD, the data
    ram[7] = (ram[7] & ~0x0f) | 0x03;
    ram[7] &= ~(1 << 7);      // write
    run(0x4c7);               // clrb RC.6: chip enable, writes the SRAM

    EXPECT_EQ(0x5a, cpu->getSRAM()[(0xfff << 4) | 0x3]);
}

// RA7 is the highest address bit of the 128 KB SRAM.
TEST_F(CpuSramTest, Ra7SelectsTheUpperHalfOfTheSram) {
    configureAllPortsAsOutputs();
    unsigned char* ram = cpu->getRAM();
    ram[7] = 1 << 6;          // chip disabled
    for (int i = 0; i < 12; i++) {
        clockPageBit(1);
    }
    ram[5] |= 1 << 7;         // RA7
    ram[8] = 0x5a;
    ram[7] = (ram[7] & ~0x0f) | 0x03;
    ram[7] &= ~(1 << 7);      // write
    run(0x4c7);               // clrb RC.6: chip enable, writes the SRAM

    EXPECT_EQ(0x5a, cpu->getSRAM()[0x10000 | (0xfff << 4) | 0x3]);
    EXPECT_EQ(0x00, cpu->getSRAM()[(0xfff << 4) | 0x3]);
}

TEST_F(CpuTest, ResetPutsTheModeRegisterBackTo1F) {
    unsigned char* ram = cpu->getRAM();
    *cpu->getPC() = 0;
    setRomValue(0, 0x055);    // mode #5
    cpu->execute();
    cpu->reset();
    *cpu->getPC() = 0;
    ram[261] = 0xaa;
    setRomValue(0, 0x042);    // mov w,m: the high nibble of W is cleared
    cpu->execute();
    EXPECT_EQ(0x0f, ram[261]);
}

// Result and flags of the instructions with a register operand. The register
// is 0x0c, the flags are Z = 4, DC = 2, C = 1 in the status register.
class CpuResultTest : public CpuTest {
protected:
    static const int Z = 4;
    static const int DC = 2;
    static const int C = 1;
    static const int NONE = -1;

    struct Case {
        const char* name;
        USHORT opcode;
        int w;               // value of W before
        int reg;             // value of the register 0x0c before
        int status;          // status register before
        int expectedW;       // NONE: not checked
        int expectedReg;     // NONE: not checked
        int flagMask;        // the flags that are checked
        int expectedFlags;
    };

    void check(const Case& c) {
        SCOPED_TRACE(c.name);
        unsigned char* ram = cpu->getRAM();
        ram[261] = c.w;
        ram[0x0c] = c.reg;
        ram[3] = c.status;
        *cpu->getPC() = 0x100;
        setRomValue(0x100, c.opcode);

        cpu->execute();

        if (c.expectedW != NONE) {
            EXPECT_EQ(c.expectedW, ram[261]);
        }
        if (c.expectedReg != NONE) {
            EXPECT_EQ(c.expectedReg, ram[0x0c]);
        }
        EXPECT_EQ(c.expectedFlags, ram[3] & c.flagMask);
    }
};

TEST_F(CpuResultTest, LogicInstructions) {
    const Case cases[] = {
        {"and fr,w", 0x160 | 0x0c, 0x0f, 0xf0, 0, NONE, 0x00, Z, Z},
        {"and fr,w keeps", 0x160 | 0x0c, 0xff, 0x3c, Z, NONE, 0x3c, Z, 0},
        {"and w,fr", 0x140 | 0x0c, 0xf0, 0x3c, 0, 0x30, NONE, Z, 0},
        {"or fr,w", 0x120 | 0x0c, 0x00, 0x00, 0, NONE, 0x00, Z, Z},
        {"or fr,w sets bits", 0x120 | 0x0c, 0x02, 0x01, Z, NONE, 0x03, Z, 0},
        {"or w,fr", 0x100 | 0x0c, 0x01, 0x02, Z, 0x03, NONE, Z, 0},
        // a zero result sets only the zero flag, page bits and carry stay
        {"or w,fr zero keeps the other bits", 0x100 | 0x0c, 0x00, 0x00, 0xa1, 0x00, NONE, 0xe7, 0xa5},
        {"xor fr,w", 0x1a0 | 0x0c, 0xff, 0xff, 0, NONE, 0x00, Z, Z},
        {"xor w,fr", 0x180 | 0x0c, 0x0f, 0xff, 0, 0xf0, NONE, Z, 0},
        {"not fr", 0x260 | 0x0c, 0, 0xff, 0, NONE, 0x00, Z, Z},
        {"not fr nonzero", 0x260 | 0x0c, 0, 0x00, Z, NONE, 0xff, Z, 0},
        {"and w,#lit", 0xe0f, 0xff, 0, 0, 0x0f, NONE, Z, 0},
        {"or w,#lit", 0xd00, 0x00, 0, 0, 0x00, NONE, Z, Z},
        {"xor w,#lit", 0xf55, 0x55, 0, 0, 0x00, NONE, Z, Z},
    };
    for (const Case& c : cases) check(c);
}

TEST_F(CpuResultTest, IncrementDecrementAndMove) {
    const Case cases[] = {
        {"inc fr wraps", 0x2a0 | 0x0c, 0, 0xff, 0, NONE, 0x00, Z, Z},
        {"inc fr", 0x2a0 | 0x0c, 0, 0x01, Z, NONE, 0x02, Z, 0},
        {"dec fr to zero", 0x0e0 | 0x0c, 0, 0x01, 0, NONE, 0x00, Z, Z},
        {"dec fr wraps", 0x0e0 | 0x0c, 0, 0x00, Z, NONE, 0xff, Z, 0},
        {"mov w,fr", 0x200 | 0x0c, 0x55, 0x00, 0, 0x00, NONE, Z, Z},
        {"mov w,/fr", 0x240 | 0x0c, 0, 0xff, 0, 0x00, NONE, Z, Z},
        {"mov w,--fr", 0x0c0 | 0x0c, 0, 0x01, 0, 0x00, 0x01, Z, Z},
        {"mov w,++fr", 0x280 | 0x0c, 0, 0xff, 0, 0x00, 0xff, Z, Z},
        {"mov w,<>fr", 0x380 | 0x0c, 0, 0x12, 0, 0x21, 0x12, 0, 0},
        {"swap fr", 0x3a0 | 0x0c, 0, 0x12, Z, NONE, 0x21, Z, Z},
        {"test fr zero", 0x220 | 0x0c, 0, 0x00, 0, NONE, 0x00, Z, Z},
        {"test fr nonzero", 0x220 | 0x0c, 0, 0x80, Z, NONE, 0x80, Z, 0},
        {"mov fr,w", 0x020 | 0x0c, 0x77, 0x00, 0, NONE, 0x77, 0, 0},
        {"clr fr", 0x060 | 0x0c, 0, 0x77, 0, NONE, 0x00, Z, Z},
        {"clr w", 0x040, 0x77, 0, 0, 0x00, NONE, Z, Z},
        {"mov w,#lit", 0xc2a, 0, 0, 0, 0x2a, NONE, 0, 0},
    };
    for (const Case& c : cases) check(c);
}

TEST_F(CpuResultTest, RotateThroughCarry) {
    const Case cases[] = {
        {"rl fr carry out", 0x360 | 0x0c, 0, 0x81, 0, NONE, 0x02, C, C},
        {"rl fr carry in", 0x360 | 0x0c, 0, 0x01, C, NONE, 0x03, C, 0},
        {"rr fr carry out", 0x320 | 0x0c, 0, 0x01, 0, NONE, 0x00, C, C},
        {"rr fr carry in", 0x320 | 0x0c, 0, 0x02, C, NONE, 0x81, C, 0},
        {"rl does not touch zero", 0x360 | 0x0c, 0, 0x00, Z, NONE, 0x00, Z, Z},
        {"mov w,<<fr carry out", 0x340 | 0x0c, 0, 0x81, 0, 0x02, 0x81, C, C},
        {"mov w,<<fr carry in", 0x340 | 0x0c, 0, 0x01, C, 0x03, 0x01, C, 0},
        {"mov w,<<fr no carry", 0x340 | 0x0c, 0, 0x40, C, 0x81, 0x40, C, 0},
        {"mov w,>>fr carry out", 0x300 | 0x0c, 0, 0x01, 0, 0x00, 0x01, C, C},
        {"mov w,>>fr carry in", 0x300 | 0x0c, 0, 0x02, C, 0x81, 0x02, C, 0},
        {"mov w,>>fr does not touch zero", 0x300 | 0x0c, 0, 0x00, Z, 0x00, 0x00, Z, Z},
    };
    for (const Case& c : cases) check(c);
}

TEST_F(CpuResultTest, AddAndSubtract) {
    const Case cases[] = {
        {"add fr,w carry and zero", 0x1e0 | 0x0c, 0x01, 0xff, 0, NONE, 0x00, Z | C, Z | C},
        {"add fr,w zero without carry", 0x1e0 | 0x0c, 0x00, 0x00, 0, NONE, 0x00, Z | C, Z},
        {"add fr,w 1 + 0xff", 0x1e0 | 0x0c, 0xff, 0x01, Z, NONE, 0x00, Z | C, Z | C},
        {"add fr,w digit carry", 0x1e0 | 0x0c, 0x01, 0x0f, 0, NONE, 0x10, Z | C | DC, DC},
        {"add fr,w plain", 0x1e0 | 0x0c, 0x01, 0x01, Z | C | DC, NONE, 0x02, Z | C | DC, 0},
        {"add w,fr carry", 0x1c0 | 0x0c, 0xf0, 0x20, 0, 0x10, NONE, C, C},
        {"add w,fr zero", 0x1c0 | 0x0c, 0x80, 0x80, 0, 0x00, NONE, Z | C, Z | C},
        // sub: C and DC are set when there is no borrow (from bit 7 and from bit 3)
        {"sub fr,w no borrow", 0x0a0 | 0x0c, 0x03, 0x05, 0, NONE, 0x02, Z | C | DC, C | DC},
        {"sub fr,w borrow", 0x0a0 | 0x0c, 0x05, 0x03, C | DC, NONE, 0xfe, Z | C | DC, 0},
        {"sub fr,w zero", 0x0a0 | 0x0c, 0x05, 0x05, 0, NONE, 0x00, Z | C | DC, Z | C | DC},
        {"sub fr,w zero flag follows the result", 0x0a0 | 0x0c, 0x00, 0x05, Z, NONE, 0x05, Z, 0},
        {"sub fr,w borrow from the low nibble only", 0x0a0 | 0x0c, 0x01, 0x10, 0, NONE, 0x0f, Z | C | DC, C},
        {"sub fr,w no borrow from the low nibble", 0x0a0 | 0x0c, 0x13, 0x25, 0, NONE, 0x12, Z | C | DC, C | DC},
        {"mov w,fr-w equal", 0x080 | 0x0c, 0x05, 0x05, 0, 0x00, 0x05, Z | C | DC, Z | C | DC},
        {"mov w,fr-w borrow", 0x080 | 0x0c, 0x06, 0x05, C | DC, 0xff, NONE, Z | C | DC, 0},
        {"mov w,fr-w borrow from the low nibble only", 0x080 | 0x0c, 0x01, 0x10, 0, 0x0f, 0x10, Z | C | DC, C},
    };
    for (const Case& c : cases) check(c);
}

TEST_F(CpuResultTest, IndirectAddressingUsesFsr) {
    unsigned char* ram = cpu->getRAM();
    ram[4] = 0x35;                    // FSR
    ram[0x35] = 0x41;
    ram[261] = 0x01;
    *cpu->getPC() = 0x100;
    setRomValue(0x100, 0x1e0 | 0x00); // add INDF,w
    cpu->execute();
    EXPECT_EQ(0x42, ram[0x35]);
}

TEST_F(CpuResultTest, RegistersAbove0x0fAreInTheBankOfFsr) {
    unsigned char* ram = cpu->getRAM();
    ram[4] = 0x30;                    // bank 3
    ram[0x3c] = 0x10;
    ram[261] = 0x01;
    *cpu->getPC() = 0x100;
    setRomValue(0x100, 0x1e0 | 0x1c); // add 0x1c,w: bank 3, register 0xc
    cpu->execute();
    EXPECT_EQ(0x11, ram[0x3c]);
}

TEST_F(CpuResultTest, SkippingInstructionsContinueBehindTheNextOne) {
    unsigned char* ram = cpu->getRAM();
    *cpu->getPC() = 0x100;
    ram[0x0c] = 1;
    setRomValue(0x100, 0x2e0 | 0x0c);   // decsz 0x0c
    cpu->execute();
    EXPECT_EQ(0x102, *cpu->getPC());
}

TEST_F(CpuResultTest, SkipPassesPageAndBankInstructions) {
    unsigned char* ram = cpu->getRAM();
    *cpu->getPC() = 0x100;
    ram[0x0c] = 1;
    setRomValue(0x100, 0x2e0 | 0x0c);   // decsz 0x0c
    setRomValue(0x101, 0x012);          // page 2, is skipped together
    setRomValue(0x102, 0x01b);          // bank 3, as well
    cpu->execute();
    EXPECT_EQ(0x104, *cpu->getPC());
}

TEST_F(CpuResultTest, PageAndBankInstructionsSelectPageAndBank) {
    unsigned char* ram = cpu->getRAM();
    ram[3] = 0x07;
    ram[4] = 0x0a;
    *cpu->getPC() = 0x100;
    setRomValue(0x100, 0x012);          // page 2
    setRomValue(0x101, 0x01b);          // bank 3
    cpu->execute();
    cpu->execute();
    EXPECT_EQ((2 << 5) | 0x07, ram[3]);
    EXPECT_EQ(0x3a, ram[4]);
}

// "bank" sets bits 6:4 of FSR. Bit 7 selects the upper or lower half of the banks and is set
// with setb FSR.7 (data sheet 5.2.2), the low nibble is not touched either.
TEST_F(CpuResultTest, BankKeepsTheUpperBlockBitOfFsr) {
    unsigned char* ram = cpu->getRAM();
    ram[4] = 0x8a;
    *cpu->getPC() = 0x100;
    setRomValue(0x100, 0x01a);          // bank 2
    cpu->execute();
    EXPECT_EQ(0xaa, ram[4]);
}

TEST_F(CpuResultTest, BankReplacesTheOldBankNumber) {
    unsigned char* ram = cpu->getRAM();
    ram[4] = 0x7f;
    *cpu->getPC() = 0x100;
    setRomValue(0x100, 0x019);          // bank 1
    cpu->execute();
    EXPECT_EQ(0x1f, ram[4]);
}

TEST_F(CpuResultTest, IreadReadsTheProgramMemoryAt_M_W) {
    unsigned char* ram = cpu->getRAM();
    *cpu->getPC() = 0x100;
    setRomValue(0x100, 0x05b);          // mode #0xb
    setRomValue(0x101, 0x041);          // iread
    setRomValue(0xb34, 0x9a5);          // the word that is read
    ram[261] = 0x34;
    cpu->execute();
    cpu->execute();
    EXPECT_EQ(0xa5, ram[261]);
    setRomValue(0x102, 0x042);          // mov w,m: the upper four bits are in M
    cpu->execute();
    EXPECT_EQ(0x9, ram[261]);
}

TEST_F(CpuResultTest, CallAndReturn) {
    unsigned char* ram = cpu->getRAM();
    *cpu->getPC() = 0x100;
    ram[3] = 0;
    setRomValue(0x100, 0x920);          // call 0x20
    setRomValue(0x20, 0x80a);           // retw #10
    cpu->execute();
    EXPECT_EQ(0x20, *cpu->getPC());
    cpu->execute();
    EXPECT_EQ(0x101, *cpu->getPC());
    EXPECT_EQ(10, ram[261]);
}

TEST_F(CpuResultTest, BitInstructionsOnRegisters) {
    unsigned char* ram = cpu->getRAM();
    ram[0x0c] = 0x00;
    *cpu->getPC() = 0x100;
    setRomValue(0x100, 0x500 | (3 << 5) | 0x0c);   // setb 0x0c.3
    cpu->execute();
    EXPECT_EQ(0x08, ram[0x0c]);
    setRomValue(0x101, 0x400 | (3 << 5) | 0x0c);   // clrb 0x0c.3
    cpu->execute();
    EXPECT_EQ(0x00, ram[0x0c]);
}

// setb/clrb on a port register: the bit goes into the latch of the port. If the
// pin is an output (its bit in the direction register is 0) the outside world
// sees it: RA is the joystick and the sound chip, RC the SRAM, RE the video.
class CpuPortBitTest : public CpuTest {
protected:
    void run(USHORT opcode) {
        *cpu->getPC() = 0x100;
        setRomValue(0x100, opcode);
        cpu->execute();
    }

    // direction register of port RB..RE: 0 is output, 1 is input
    void setDirection(int port, int value) {
        cpu->getRAM()[261] = value;
        run(0x05f);           // mode #0xf
        run(0x005 + port);    // mov !r<port>,w
    }
};

TEST_F(CpuPortBitTest, SetbOnAnOutputOfPortBChangesTheLatch) {
    setDirection(1, 0x00);
    cpu->getRAM()[6] = 0x00;
    run(0x500 | (3 << 5) | 6);   // setb RB.3
    EXPECT_EQ(0x08, cpu->getRAM()[6]);
}

TEST_F(CpuPortBitTest, ClrbOnAnOutputOfPortDChangesTheLatch) {
    setDirection(3, 0x00);
    cpu->getRAM()[8] = 0xff;
    run(0x400 | (7 << 5) | 8);   // clrb RD.7
    EXPECT_EQ(0x7f, cpu->getRAM()[8]);
}

TEST_F(CpuPortBitTest, SetbOnAnOutputOfPortEPutsTheNewColourOnTheVideo) {
    setDirection(4, 0x00);
    cpu->getRAM()[9] = 0x01;
    run(0x500 | (4 << 5) | 9);   // setb RE.4
    EXPECT_EQ(0x11, cpu->getRAM()[9]);
    EXPECT_EQ(1, videoMock->calls);
    EXPECT_EQ(0x11, videoMock->lastValue);
}

TEST_F(CpuPortBitTest, ClrbOnAnOutputOfPortEPutsTheNewColourOnTheVideo) {
    setDirection(4, 0x00);
    cpu->getRAM()[9] = 0xff;
    run(0x400 | (0 << 5) | 9);   // clrb RE.0
    EXPECT_EQ(1, videoMock->calls);
    EXPECT_EQ(0xfe, videoMock->lastValue);
}

TEST_F(CpuPortBitTest, SetbOnAnInputOfPortEDoesNotReachTheVideo) {
    setDirection(4, 0x10);       // RE.4 is an input
    cpu->getRAM()[9] = 0x01;
    run(0x500 | (4 << 5) | 9);   // setb RE.4
    EXPECT_EQ(0, videoMock->calls);
}

TEST_F(CpuPortBitTest, SetbOnAnOutputPinWithAHigherBitNumberStillCounts) {
    // the direction register has 0 for outputs: only bit 5 is an input
    setDirection(4, 0x20);
    cpu->getRAM()[9] = 0x00;
    run(0x500 | (6 << 5) | 9);   // setb RE.6, an output
    EXPECT_EQ(1, videoMock->calls);
}

// The real time clock/counter (RTCC, register 1) and its prescaler, as in the SX52
// data sheet (section 4.3 and 9): the OPTION register is 0xFF after a reset, which
// gives the prescaler to the watchdog, so the RTCC counts every cycle. With PSA = 0
// the prescaler divides by 2 << PS. Writing the RTCC clears the prescaler.
class CpuTimerTest : public CpuTest {
protected:
    enum {
        NOP = 0x000,
        CLR_RTCC = 0x061,       // clr 1
        MOV_RTCC_W = 0x021,     // mov 1,w
        MOV_OPTION_W = 0x002    // mov !option,w
    };

    // loads the program at address 0 and clears the rest
    void program(const std::vector<USHORT>& words) {
        for (int i = 0; i < 64; i++) {
            setRomValue(i, i < (int)words.size() ? words[i] : NOP);
        }
        *cpu->getPC() = 0;
    }

    // mov w,#option; mov !option,w
    std::vector<USHORT> setOption(int option) {
        std::vector<USHORT> words;
        words.push_back(0xc00 | option);
        words.push_back(MOV_OPTION_W);
        return words;
    }

    void run(int instructions) {
        for (int i = 0; i < instructions; i++) {
            cpu->execute();
        }
    }

    int rtcc() {
        return cpu->getRAM()[1];
    }
};

// 0xFF has RTS set: after a reset the RTCC counts edges on its pin, not instruction
// cycles. A program has to write the OPTION register first.
TEST_F(CpuTimerTest, AfterAResetTheRtccDoesNotCountInstructionCycles) {
    program({});
    run(100);
    EXPECT_EQ(0, rtcc());
}

TEST_F(CpuTimerTest, AResetPutsTheOptionRegisterBackToAllOnes) {
    program(setOption(0xcf));      // counts every cycle
    run(30);
    ASSERT_GT(rtcc(), 0);
    cpu->reset();
    program({});
    run(100);
    EXPECT_EQ(0, rtcc());
}

TEST_F(CpuTimerTest, ThePrescalerDividesByTheRatioOfTheOptionRegister) {
    std::vector<USHORT> words = setOption(0xc3);   // PSA = 0, PS = 3: 1:16
    words.push_back(CLR_RTCC);
    program(words);
    run(3 + 15);
    EXPECT_EQ(0, rtcc());
    run(1);
    EXPECT_EQ(1, rtcc());
}

TEST_F(CpuTimerTest, ThePrescalerKeepsWhatIsLeftOverWhenItCounts) {
    // a jump takes 3 cycles, 16 is no multiple of it
    std::vector<USHORT> words = setOption(0xc3);
    words.push_back(CLR_RTCC);
    words.push_back(0xa00 | 3);                    // jmp 3
    program(words);
    run(3 + 17);                                    // 1 + 16 * 3 = 49 cycles are counted
    EXPECT_EQ(3, rtcc());
}

TEST_F(CpuTimerTest, WithTheMaximumRatioTheRtccCountsEvery256Cycles) {
    std::vector<USHORT> words = setOption(0xc7);   // 1:256
    words.push_back(CLR_RTCC);
    program(words);
    run(3 + 255);
    EXPECT_EQ(0, rtcc());
    run(1);
    EXPECT_EQ(1, rtcc());
}

TEST_F(CpuTimerTest, WhenThePrescalerBelongsToTheWatchdogTheRtccCountsEveryCycle) {
    std::vector<USHORT> words = setOption(0xcf);   // PSA = 1
    words.push_back(CLR_RTCC);
    program(words);
    run(3 + 10);
    EXPECT_EQ(10, rtcc());
}

TEST_F(CpuTimerTest, WritingTheRtccClearsThePrescaler) {
    std::vector<USHORT> words = setOption(0xc3);
    words.push_back(CLR_RTCC);
    for (int i = 0; i < 10; i++) {
        words.push_back(NOP);
    }
    words.push_back(0x040);                         // clr w
    words.push_back(MOV_RTCC_W);                    // writes 0, the prescaler counted 11 cycles
    program(words);
    run(3 + 10 + 1 + 1);
    run(15);
    EXPECT_EQ(0, rtcc());
    run(1);
    EXPECT_EQ(1, rtcc());
}

TEST_F(CpuTimerTest, TheClearInstructionsOfTheOtherRegisterWritesAlsoClearThePrescaler) {
    // inc RTCC is a write as well
    std::vector<USHORT> words = setOption(0xc3);
    words.push_back(CLR_RTCC);
    for (int i = 0; i < 10; i++) {
        words.push_back(NOP);
    }
    words.push_back(0x2a0 | 1);                     // inc 1
    program(words);
    run(3 + 10 + 1);
    EXPECT_EQ(1, rtcc());
    run(15);
    EXPECT_EQ(1, rtcc());
    run(1);
    EXPECT_EQ(2, rtcc());
}

TEST_F(CpuTimerTest, AnExternalClockSourceNeverCounts) {
    std::vector<USHORT> words = setOption(0xe3);   // RTS = 1: counts on the RTCC pin
    words.push_back(CLR_RTCC);
    program(words);
    run(3 + 500);
    EXPECT_EQ(0, rtcc());
}

// The CF bit of the FUSEX word (bit 6): if it is 0, the carry flag is an input of ADD and SUB
// (data sheet, section 4.6 and the instruction set table). SUB then subtracts the complement
// of the carry, the borrow. The XGS programs that do 16 bit arithmetic rely on it.
class CpuCarryInputTest : public CpuResultTest {
protected:
    void SetUp() override {
        CpuResultTest::SetUp();
        cpu->setFusex(0xfbf);        // CF = 0
    }
};

TEST_F(CpuCarryInputTest, TheFusexWordWithCfZeroSwitchesTheCarryInputOn) {
    const Case cases[] = {
        {"add fr,w with carry", 0x1e0 | 0x0c, 0x01, 0x01, C, NONE, 0x03, C, 0},
        {"add fr,w without carry flag", 0x1e0 | 0x0c, 0x01, 0x01, 0, NONE, 0x02, C, 0},
        {"add w,fr with carry", 0x1c0 | 0x0c, 0x01, 0x01, C, 0x03, NONE, C, 0},
        {"add fr,w the carry in makes a carry out", 0x1e0 | 0x0c, 0x00, 0xff, C, NONE, 0x00, Z | C, Z | C},
    };
    for (const Case& c : cases) check(c);
}

TEST_F(CpuCarryInputTest, SubtractsTheBorrowThatIsTheComplementOfTheCarry) {
    const Case cases[] = {
        {"sub fr,w carry set is no borrow", 0x0a0 | 0x0c, 0x03, 0x05, C, NONE, 0x02, C, C},
        {"sub fr,w carry clear is a borrow", 0x0a0 | 0x0c, 0x03, 0x05, 0, NONE, 0x01, C, C},
        {"sub fr,w the borrow can make a borrow", 0x0a0 | 0x0c, 0x05, 0x05, 0, NONE, 0xff, C, 0},
        {"mov w,fr-w carry clear is a borrow", 0x080 | 0x0c, 0x03, 0x05, 0, 0x01, NONE, C, C},
        {"mov w,fr-w carry set is no borrow", 0x080 | 0x0c, 0x03, 0x05, C, 0x02, NONE, C, C},
    };
    for (const Case& c : cases) check(c);
}

TEST_F(CpuCarryInputTest, TheJumpWithAddToTheProgramCounterIgnoresTheCarry) {
    USHORT* pc = cpu->getPC();
    unsigned char* ram = cpu->getRAM();
    *pc = 0xa00;
    ram[261] = 0x03;
    ram[3] |= C;
    setRomValue(0xa00, 0x1e2);
    cpu->execute();
    EXPECT_EQ(0xa04, *pc);
}

TEST_F(CpuResultTest, WithTheCfBitSetTheCarryIsNoInput) {
    cpu->setFusex(0xfff);            // CF = 1, the default
    check({"add fr,w", 0x1e0 | 0x0c, 0x01, 0x01, C, NONE, 0x02, C, 0});
    check({"sub fr,w", 0x0a0 | 0x0c, 0x03, 0x05, 0, NONE, 0x02, C, C});
}

// The MODE register of the SX52 (data sheet 3.3): five bits, 0x1F after a reset. It selects what
// "mov !rx,W" accesses: the low four bits pick the control register (0xF the data direction, 0xE
// the pull-ups, 0xD the input levels, 0xC the Schmitt triggers) and bit 4 the direction of the
// transfer: 1 writes W to the register, 0 reads the register into W. "mov M,#lit" only sets the
// lower four bits, "mov M,W" the lower five, "mov W,M" clears the high nibble of W.
// All control registers are 0xFF after a reset: every pin is an input.
class CpuModeTest : public CpuTest {
protected:
    enum {
        MOV_W_M = 0x042,
        MOV_M_W = 0x043,
        MOV_RA_W = 0x005       // mov !ra,w, port n is 5 + n
    };

    void run(USHORT opcode) {
        *cpu->getPC() = 0x100;
        setRomValue(0x100, opcode);
        cpu->execute();
    }

    // W = value, MODE = mode (all five bits)
    void setMode(int mode) {
        cpu->getRAM()[261] = mode;
        run(MOV_M_W);
    }

    int w() {
        return cpu->getRAM()[261];
    }

    void setW(int value) {
        cpu->getRAM()[261] = value;
    }
};

TEST_F(CpuModeTest, TheModeRegisterIs1FAfterAReset) {
    run(MOV_W_M);
    EXPECT_EQ(0x0f, w());        // the high nibble of W is cleared, bit 4 is not visible
    // but it is there: mov !ra,w writes
    setW(0x3c);
    run(MOV_RA_W);
    setMode(0x0f);               // now read
    setW(0);
    run(MOV_RA_W);
    EXPECT_EQ(0x3c, w());
}

TEST_F(CpuModeTest, AllControlRegistersAreAllOnesAfterAReset) {
    setMode(0x0f);
    for (int port = 0; port < 5; port++) {
        setW(0);
        run(MOV_RA_W + port);
        EXPECT_EQ(0xff, w()) << "direction of port " << port;
    }
    setMode(0x0e);
    setW(0);
    run(MOV_RA_W);
    EXPECT_EQ(0xff, w()) << "pull-ups";
}

TEST_F(CpuModeTest, MovMWSetsTheLowerFiveBitsAndMovWMReadsTheLowerFour) {
    setMode(0xff);
    run(MOV_W_M);
    EXPECT_EQ(0x0f, w());
    setMode(0x05);               // bit 4 is 0 now
    run(MOV_W_M);
    EXPECT_EQ(0x05, w());
}

TEST_F(CpuModeTest, MovMLiteralKeepsBitFour) {
    setMode(0x00);               // read
    run(0x05f);                  // mov m,#0xf: the low four bits only
    setW(0);
    run(MOV_RA_W);               // still a read: W = direction (0xff)
    EXPECT_EQ(0xff, w());
}

TEST_F(CpuModeTest, WritingTheDirectionRegisterNeedsBitFourOfTheMode) {
    setMode(0x1f);               // write the direction, like Rem-Pac does with "mov M,W" and TRIS = $1F
    setW(0x0c);
    run(MOV_RA_W);
    setMode(0x0f);               // read it back
    setW(0);
    run(MOV_RA_W);
    EXPECT_EQ(0x0c, w());
}

TEST_F(CpuModeTest, WithBitFourClearMovRxWOnlyReadsAndLeavesTheRegisterAlone) {
    setMode(0x1f);
    setW(0x0c);
    run(MOV_RA_W);
    setMode(0x0f);
    setW(0xaa);
    run(MOV_RA_W);               // reads 0x0c into W, does not write 0xaa
    EXPECT_EQ(0x0c, w());
    run(MOV_RA_W);
    EXPECT_EQ(0x0c, w());
}

TEST_F(CpuModeTest, EachPortHasItsOwnRegister) {
    setMode(0x1f);
    for (int port = 0; port < 5; port++) {
        setW(0x10 + port);
        run(MOV_RA_W + port);
    }
    setMode(0x0f);
    for (int port = 0; port < 5; port++) {
        setW(0);
        run(MOV_RA_W + port);
        EXPECT_EQ(0x10 + port, w()) << port;
    }
}

TEST_F(CpuModeTest, ThePullupRegistersAreWrittenAndReadSeparately) {
    setMode(0x1e);
    setW(0x03);
    run(MOV_RA_W);
    setMode(0x1f);
    setW(0x77);
    run(MOV_RA_W);               // the direction register is another one
    setMode(0x0e);
    setW(0);
    run(MOV_RA_W);
    EXPECT_EQ(0x03, w());
    setMode(0x0f);
    run(MOV_RA_W);
    EXPECT_EQ(0x77, w());
}

TEST_F(CpuModeTest, TheInputLevelAndSchmittTriggerRegistersAreSeparateRegisters) {
    setMode(0x1d);               // input levels
    setW(0x11);
    run(MOV_RA_W + 1);
    setMode(0x1c);               // Schmitt triggers
    setW(0x22);
    run(MOV_RA_W + 1);
    setMode(0x0d);
    run(MOV_RA_W + 1);
    EXPECT_EQ(0x11, w());
    setMode(0x0c);
    run(MOV_RA_W + 1);
    EXPECT_EQ(0x22, w());
}

TEST_F(CpuModeTest, AResetPutsTheModeAndTheControlRegistersBack) {
    setMode(0x1f);
    setW(0x00);
    run(MOV_RA_W);
    setMode(0x00);
    cpu->reset();
    setMode(0x0f);               // read the direction of port A
    setW(0);
    run(MOV_RA_W);
    EXPECT_EQ(0xff, w());
    run(MOV_W_M);
    cpu->reset();
    run(MOV_W_M);
    EXPECT_EQ(0x0f, w());
}

TEST_F(CpuModeTest, IreadReplacesTheLowerFourBitsOfTheModeAndKeepsBitFour) {
    setMode(0x1b);               // M = 0xb for the address, bit 4 set
    setW(0x34);
    setRomValue(0xb34, 0x9a5);
    run(0x041);                  // iread: W = 0xa5 and M = 9 (the upper bits of the word)
    EXPECT_EQ(0xa5, w());
    run(MOV_W_M);
    EXPECT_EQ(0x09, w());
    run(0x05f);                  // mov m,#0xf: bit 4 is still set, so this writes
    setW(0x42);
    run(MOV_RA_W);
    setMode(0x0f);
    setW(0);
    run(MOV_RA_W);
    EXPECT_EQ(0x42, w());
}

TEST_F(CpuModeTest, ThePortBitsOfAPinAreOnlySeenByTheOutsideWorldIfItIsAnOutput) {
    // after a reset all pins are inputs: setb RE.4 changes the latch but not the video
    cpu->getRAM()[9] = 0x01;
    run(0x500 | (4 << 5) | 9);
    EXPECT_EQ(0x11, cpu->getRAM()[9]);
    EXPECT_EQ(0, videoMock->calls);
    // an output pin reaches it
    setMode(0x1f);
    setW(0x00);
    run(MOV_RA_W + 4);
    run(0x500 | (5 << 5) | 9);
    EXPECT_EQ(1, videoMock->calls);
}

// Every setting of the prescaler: the RTCC counts the cycles divided by the ratio.
TEST_F(CpuTimerTest, EveryPrescalerSettingCountsTheCyclesDividedByItsRatio) {
    for (int psa = 0; psa < 2; psa++) {
        for (int ps = 0; ps < 8; ps++) {
            SCOPED_TRACE(testing::Message() << "PSA " << psa << " PS " << ps);
            cpu->reset();
            std::vector<USHORT> words = setOption(0xc0 | (psa << 3) | ps);
            words.push_back(CLR_RTCC);
            program(words);
            // program() clears only 64 words, the test runs more instructions
            for (int i = 64; i < 1100; i++) {
                setRomValue(i, NOP);
            }
            run(3 + 1000);
            int ratio = psa ? 1 : (2 << ps);
            EXPECT_EQ((1000 / ratio) & 0xff, rtcc());
        }
    }
}
