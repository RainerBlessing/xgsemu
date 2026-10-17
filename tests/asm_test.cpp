#include <gtest/gtest.h>
#include "asm.h"
#include "disasm.h"

namespace {
bool assembles(const std::string& text, unsigned short expected) {
    unsigned short opcode = 0;
    return assemble(text, &opcode) && opcode == expected;
}
}

TEST(AssembleTest, MovWithLiteral) {
    EXPECT_TRUE(assembles("mov w, #a", 0xc0a));
}

TEST(AssembleTest, JumpAndCall) {
    EXPECT_TRUE(assembles("jmp 1ff", 0xbff));
    EXPECT_TRUE(assembles("call 42", 0x942));
}

TEST(AssembleTest, RegisterAndBit) {
    EXPECT_TRUE(assembles("add 5, w", 0x1e5));
    EXPECT_TRUE(assembles("setb 5.3", 0x565));
}

TEST(AssembleTest, InstructionWithoutOperand) {
    EXPECT_TRUE(assembles("nop", 0x000));
    EXPECT_TRUE(assembles("ret", 0x00c));
}

TEST(AssembleTest, IgnoresCaseAndWhitespace) {
    EXPECT_TRUE(assembles("  MOV   W,#A  ", 0xc0a));
    EXPECT_TRUE(assembles("Clr !WDT", 0x004));
}

TEST(AssembleTest, AcceptsLeadingZerosAndHexPrefix) {
    EXPECT_TRUE(assembles("mov w, #0a", 0xc0a));
    EXPECT_TRUE(assembles("mov w, #0x0a", 0xc0a));
    EXPECT_TRUE(assembles("jmp 0x1ff", 0xbff));
}

TEST(AssembleTest, IgnoresComment) {
    EXPECT_TRUE(assembles("nop ; do nothing", 0x000));
}

TEST(AssembleTest, RejectsUnknownMnemonic) {
    unsigned short opcode = 0;
    EXPECT_FALSE(assemble("frobnicate 5", &opcode));
}

TEST(AssembleTest, RejectsOperandOutOfRange) {
    unsigned short opcode = 0;
    EXPECT_FALSE(assemble("mov w, #100", &opcode));
    EXPECT_FALSE(assemble("jmp 200", &opcode));
    EXPECT_FALSE(assemble("setb 5.8", &opcode));
    EXPECT_FALSE(assemble("add 20, w", &opcode));
}

TEST(AssembleTest, RejectsEmptyLine) {
    unsigned short opcode = 0;
    EXPECT_FALSE(assemble("", &opcode));
}

// What the disassembler writes, the assembler reads again.
TEST(AssembleTest, ReadsEveryDisassembledInstruction) {
    for (unsigned short op = 0; op < 0x1000; op++) {
        std::string text = disassemble(op);
        if (text == "unused" || text.compare(0, 8, "not impl") == 0) {
            continue;
        }
        unsigned short opcode = 0xffff;
        ASSERT_TRUE(assemble(text, &opcode)) << text;
        EXPECT_EQ(op, opcode) << text;
    }
}
