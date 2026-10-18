#include <gtest/gtest.h>
#include <string.h>
#include <vector>
#include "../src/Video.h"
#include "../src/XgsBoard.h"
#include "cpu.h"
#include "../src/filehandler.h"

#ifndef EXAMPLES_DIR
#define EXAMPLES_DIR "examples"
#endif

// The wait loops "djnz counter, self" (decsz + jmp, sometimes with a page
// instruction in between) are the delays of the video timing. The CPU does
// all their rounds in one step. This must not change anything but the number
// of execute() calls: cycles, registers, flags and the RTCC stay exactly the
// same as when every round is executed on its own.
class WaitLoopTest : public ::testing::Test {
protected:
    enum {
        NOP = 0x000,
        MOV_OPTION_W = 0x002,
        PAGE_0 = 0x010,
        JMP_BASE = 0xa00
    };
    static const int COUNTER = 0x0c;
    static const int LOOP = 0x20;       // address of the decsz

    // the wait loops are about cycles, not about what is on the pins

    struct Run {
        int cycles;
        int pc;
        int counter;
        int status;
        int rtcc;
        int w;
        int executed;       // number of execute() calls
    };

    // mov w,#option; mov !option,w; the RTCC counts every cycle
    // loop:  decsz counter; [page 0;] jmp loop; nop; nop
    Run run(bool fast, int counter, bool withPage, int counterRegister = COUNTER) {
        Cpu cpu(NULL);
        cpu.setFastLoops(fast);
        USHORT* rom = cpu.getROM();
        for (int i = 0; i < 128; i++) {
            rom[i] = NOP;
        }
        rom[0] = 0xc00 | 0xcf;
        rom[1] = MOV_OPTION_W;
        rom[2] = 0xc00 | counter;
        rom[3] = 0x020 | counterRegister;       // mov counterRegister,w
        rom[4] = 0xa00 | LOOP;                  // jmp loop (nothing is skipped)
        int a = LOOP;
        rom[a++] = 0x2e0 | counterRegister;      // decsz
        if (withPage) {
            rom[a++] = PAGE_0;
        }
        rom[a++] = JMP_BASE | LOOP;
        rom[a++] = NOP;
        rom[a++] = NOP;
        int endAddress = a;
        *cpu.getPC() = 0;

        Run r = {};
        // run until the two nops behind the loop are done
        while (*cpu.getPC() != endAddress && r.executed < 100000) {
            cpu.execute();
            r.executed++;
        }
        r.cycles = cpu.getCycle();
        r.pc = *cpu.getPC();
        r.counter = cpu.getRAM()[counterRegister];
        r.status = cpu.getRAM()[3];
        r.rtcc = cpu.getRAM()[1];
        r.w = cpu.getRAM()[261];
        return r;
    }

    void expectSameAsStepping(int counter, bool withPage) {
        SCOPED_TRACE(testing::Message() << "counter " << counter << (withPage ? " with page" : ""));
        Run stepped = run(false, counter, withPage);
        Run fast = run(true, counter, withPage);
        EXPECT_EQ(stepped.cycles, fast.cycles);
        EXPECT_EQ(stepped.pc, fast.pc);
        EXPECT_EQ(stepped.counter, fast.counter);
        EXPECT_EQ(stepped.status, fast.status);
        EXPECT_EQ(stepped.rtcc, fast.rtcc);
        EXPECT_EQ(stepped.w, fast.w);
    }
};

TEST_F(WaitLoopTest, SameResultAsSteppingForManyCounters) {
    const int counters[] = {1, 2, 3, 5, 19, 100, 254, 255, 0};   // 0 counts 256 rounds
    for (int counter : counters) {
        expectSameAsStepping(counter, false);
    }
}

TEST_F(WaitLoopTest, SameResultAsSteppingWithAPageInstructionInTheLoop) {
    const int counters[] = {1, 2, 3, 5, 19, 100, 255, 0};
    for (int counter : counters) {
        expectSameAsStepping(counter, true);
    }
}

TEST_F(WaitLoopTest, TheLoopLeavesTheCounterAtZero) {
    EXPECT_EQ(0, run(true, 50, false).counter);
}

TEST_F(WaitLoopTest, FewerInstructionsAreExecutedThanRounds) {
    EXPECT_GT(run(false, 200, false).executed, 400);
    EXPECT_LT(run(true, 200, false).executed, 20);
}

// Registers with a side effect (ports, RTCC, FSR) are never taken shortcut.
TEST_F(WaitLoopTest, ASpecialRegisterAsCounterIsSteppedOnItsOwn) {
    // register 4 is FSR: decsz FSR
    Run stepped = run(false, 30, false, 4);
    Run fast = run(true, 30, false, 4);
    EXPECT_EQ(stepped.cycles, fast.cycles);
    EXPECT_EQ(stepped.executed, fast.executed);
}

// A real program: the lock step comparison of a CPU with and without the
// shortcut. Both have to be in the same state whenever the fast one has
// completed an instruction, and show the same picture in the end.
class WaitLoopProgramTest : public ::testing::TestWithParam<const char*> {
};

TEST_P(WaitLoopProgramTest, RunsLikeTheSteppedCpu) {
    std::string path = std::string(EXAMPLES_DIR) + "/" + GetParam();
    Video videoStepped, videoFast;
    XgsBoard boardStepped(&videoStepped, NULL), boardFast(&videoFast, NULL);
    Cpu stepped(&boardStepped), fast(&boardFast);
    stepped.setFastLoops(false);
    fast.setFastLoops(true);
    ASSERT_TRUE(FileHandler::readFile(path.c_str(), stepped.getROM()));
    ASSERT_TRUE(FileHandler::readFile(path.c_str(), fast.getROM()));

    const unsigned int cycles = 4000000;   // about 50 ms of emulated time
    int steps = 0;
    while (fast.getCycle() < (int)cycles) {
        fast.execute();
        steps++;
        while ((int)(stepped.getCycle() - fast.getCycle()) < 0) {
            stepped.execute();
        }
        ASSERT_EQ(stepped.getCycle(), fast.getCycle()) << "step " << steps;
        ASSERT_EQ(*stepped.getPC(), *fast.getPC()) << "step " << steps;
        ASSERT_EQ(0, memcmp(stepped.getRAM(), fast.getRAM(), 262)) << "step " << steps;
    }
    EXPECT_EQ(0, memcmp(boardStepped.getSRAM(), boardFast.getSRAM(), XgsBoard::SRAM_SIZE));
    EXPECT_EQ(0, memcmp(videoStepped.pixels(), videoFast.pixels(), videoStepped.pitch() * videoStepped.height()));
}

INSTANTIATE_TEST_SUITE_P(Examples, WaitLoopProgramTest,
    ::testing::Values("rem_pac_01.hex", "kj_pong_01.hex", "mic_plasma_01.hex", "racer_01.hex",
                      "starfield_01.hex", "rem_tetris_01.hex", "mic_flags_01.hex", "kj_3d_01.hex"));
