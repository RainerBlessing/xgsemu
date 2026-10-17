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
