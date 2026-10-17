#include <gtest/gtest.h>
#include "disasm.h"

TEST(DisassembleTest, EmptyRomCellIsUnused) {
    EXPECT_EQ("unused", disassemble(0xfff));
}

TEST(DisassembleTest, MovWithLiteral) {
    EXPECT_EQ("mov w, #a", disassemble(0xc0a));
}

TEST(DisassembleTest, JumpShowsNinebitAddress) {
    EXPECT_EQ("jmp 1ff", disassemble(0xbff));
}

TEST(DisassembleTest, AddWToRegister) {
    EXPECT_EQ("add 5, w", disassemble(0x1e5));
}

TEST(DisassembleTest, SetBitShowsRegisterAndBit) {
    EXPECT_EQ("setb 5.3", disassemble(0x565));
}

TEST(DisassembleTest, CallShowsEightbitAddress) {
    EXPECT_EQ("call 42", disassemble(0x942));
}

TEST(DisassembleTest, ShiftRightIsNotShownAsShiftLeft) {
    EXPECT_EQ("mov w, >>5", disassemble(0x305));
    EXPECT_EQ("mov w, <<5", disassemble(0x345));
}

TEST(DisassembleTest, SubHasACommaBeforeW) {
    EXPECT_EQ("sub 5, w", disassemble(0x0a5));
}

TEST(DisassembleTest, ModeRegisterInstructions) {
    EXPECT_EQ("mov w, m", disassemble(0x042));
    EXPECT_EQ("mov m, w", disassemble(0x043));
    EXPECT_EQ("mov m, #a", disassemble(0x05a));
}

TEST(DisassembleTest, PortControlNamesThePort) {
    EXPECT_EQ("mov !ra, w", disassemble(0x005));
    EXPECT_EQ("mov !re, w", disassemble(0x009));
}

TEST(DisassembleTest, ControlInstructionsAreLowerCase) {
    EXPECT_EQ("clr !wdt", disassemble(0x004));
    EXPECT_EQ("iread", disassemble(0x041));
    EXPECT_EQ("sleep", disassemble(0x003));
    EXPECT_EQ("rl 5", disassemble(0x365));
    EXPECT_EQ("rr 5", disassemble(0x325));
}

TEST(DisassembleTest, ReturnInstructions) {
    EXPECT_EQ("ret", disassemble(0x00c));
    EXPECT_EQ("retp", disassemble(0x00d));
    EXPECT_EQ("reti", disassemble(0x00e));
    EXPECT_EQ("retiw", disassemble(0x00f));
    EXPECT_EQ("retw #5", disassemble(0x805));
}
