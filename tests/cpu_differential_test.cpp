// Differential test for the refactoring of Cpu::execute.
//
// CpuReference is a frozen copy of the Cpu from before the refactoring. Both
// get the same state, run the same instructions and must end up in exactly the
// same state, also in what they send to the video. The test needs the internals
// of both classes, so they are made accessible here.
#include <gtest/gtest.h>
#include <SDL.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#define private public
#include "../src/cpu.h"
#include "../src/Video.h"
#include "cpu_reference.h"
#undef private

#include "../src/filehandler.h"

#ifndef EXAMPLES_DIR
#define EXAMPLES_DIR "examples"
#endif

namespace {

class Rng {
public:
    explicit Rng(uint64_t seed) : state(seed * 2685821657736338717ULL + 88172645463325252ULL) {
        for (int i = 0; i < 4; i++) {
            next();
        }
    }

    uint32_t next() {
        state ^= state << 13;
        state ^= state >> 7;
        state ^= state << 17;
        return (uint32_t)(state >> 11);
    }

    uint32_t below(uint32_t limit) {
        return next() % limit;
    }

private:
    uint64_t state;
};

// Records what the CPU sends to the video instead of drawing it.
class RecordingVideo : public Video {
public:
    void changeVideo(int cycle, unsigned char colour) override {
        calls.push_back(std::make_pair(cycle, (int)colour));
    }

    std::vector<std::pair<int, int> > calls;
};

const int STEPS = 3;                 // instructions per random start state
const int STATES_PER_OPCODE = 48;
const int ROM_SIZE = 4096;

// Gives the same random state to a Cpu and to a CpuReference. The two classes
// have identical members, so one template serves both.
template <class C>
void randomizeState(C& cpu, uint64_t seed, unsigned opcode) {
    // the program memory is the same for all start states of one opcode
    Rng rom(opcode + 1);
    for (int i = 0; i < ROM_SIZE; i++) {
        cpu.ROM[i] = rom.next() & 0xfff;
    }

    Rng r(seed);
    for (int i = 0; i < 262; i++) {
        cpu.RAM[i] = r.next() & 0xff;
    }
    cpu.pc = r.below(0xf00);
    cpu.ROM[cpu.pc] = opcode;
    // sometimes a page/bank instruction follows, which a skip has to pass over
    if (r.below(4) == 0) {
        cpu.ROM[cpu.pc + 1] = 0x010 + r.below(16);
    }
    // the stack depth leaves room for three calls or three returns
    cpu.sp = cpu.Stack + 3 + r.below(3);
    for (int i = 0; i < 8; i++) {
        cpu.Stack[i] = r.below(ROM_SIZE);
    }
    cpu.mode = r.next() & 0xff;
    cpu.option = r.next() & 0xff;
    for (int i = 0; i < 5; i++) {
        cpu.Tris[i] = r.next() & 0xff;
    }
    cpu.cycle = r.next();
    cpu.cycle2 = cpu.cycle - r.below(8);
    cpu.prescaler = r.below(40);
    cpu.overshoot = 0;
    cpu.joystickr = r.next() & 0xffff;
    cpu.lastRa = r.next() & 0xff;
}

template <class C>
void randomizeNextInstruction(C& cpu, uint64_t seed) {
    Rng r(seed);
    cpu.ROM[cpu.pc] = r.next() & 0xfff;
}

// Returns an empty string if both are in the same state.
template <class A, class B>
std::string compareState(const A& a, const B& b) {
    std::ostringstream out;
    for (int i = 0; i < 262; i++) {
        if (a.RAM[i] != b.RAM[i]) {
            out << "RAM[" << i << "] " << (int)a.RAM[i] << " but expected " << (int)b.RAM[i] << "; ";
        }
    }
    if (a.pc != b.pc) {
        out << "pc " << a.pc << " but expected " << b.pc << "; ";
    }
    for (int i = 0; i < 8; i++) {
        if (a.Stack[i] != b.Stack[i]) {
            out << "Stack[" << i << "]; ";
        }
    }
    if ((a.sp - a.Stack) != (b.sp - b.Stack)) {
        out << "sp " << (a.sp - a.Stack) << " but expected " << (b.sp - b.Stack) << "; ";
    }
    if (a.mode != b.mode) {
        out << "mode; ";
    }
    if (a.option != b.option) {
        out << "option; ";
    }
    for (int i = 0; i < 5; i++) {
        if (a.Tris[i] != b.Tris[i]) {
            out << "Tris[" << i << "]; ";
        }
    }
    if (a.cycle != b.cycle) {
        out << "cycle " << a.cycle << " but expected " << b.cycle << "; ";
    }
    if (a.cycle2 != b.cycle2) {
        out << "cycle2; ";
    }
    if (a.prescaler != b.prescaler) {
        out << "prescaler; ";
    }
    if (a.overshoot != b.overshoot) {
        out << "overshoot; ";
    }
    if (a.lastRa != b.lastRa) {
        out << "lastRa; ";
    }
    return out.str();
}

// The video side: the fragments the line timing has seen and the picture.
std::string compareVideo(const Video& a, const Video& b) {
    std::ostringstream out;
    if (memcmp(a.slfragments, b.slfragments, sizeof(a.slfragments)) != 0) {
        out << "video fragments; ";
    }
    if (a.fragmentcounter != b.fragmentcounter || a.yc != b.yc || a.frame != b.frame ||
        a.vsync != b.vsync || a.line != b.line || a.x != b.x || a.y != b.y) {
        out << "video line state; ";
    }
    SDL_Surface* sa = a.screen;
    SDL_Surface* sb = b.screen;
    if (memcmp(sa->pixels, sb->pixels, sa->pitch * sa->h) != 0) {
        out << "picture; ";
    }
    return out.str();
}

}  // namespace

// Every opcode, many random states: after each instruction the CPU has to be
// in the same state as the reference and must have sent the same to the video.
TEST(CpuDifferentialTest, EveryOpcodeBehavesLikeTheReference) {
    for (unsigned opcode = 0; opcode < 0x1000; opcode++) {
        RecordingVideo videoA;
        RecordingVideo videoB;
        Cpu a(&videoA);
        CpuReference b(&videoB);

        for (int n = 0; n < STATES_PER_OPCODE; n++) {
            uint64_t seed = (uint64_t)opcode * 1000 + n;
            memset(a.SRAM, 0, sizeof(a.SRAM));
            memset(b.SRAM, 0, sizeof(b.SRAM));
            randomizeState(a, seed, opcode);
            randomizeState(b, seed, opcode);
            videoA.calls.clear();
            videoB.calls.clear();

            for (int step = 0; step < STEPS; step++) {
                if (step > 0) {
                    randomizeNextInstruction(a, seed * 7 + step);
                    randomizeNextInstruction(b, seed * 7 + step);
                }
                a.execute();
                b.execute();

                std::string difference = compareState(a, b);
                if (videoA.calls != videoB.calls) {
                    difference += "video calls; ";
                }
                ASSERT_EQ("", difference) << "opcode 0x" << std::hex << opcode << std::dec
                                          << ", state " << n << ", step " << step;
            }
        }
        ASSERT_EQ(0, memcmp(a.SRAM, b.SRAM, sizeof(a.SRAM))) << "SRAM, opcode 0x" << std::hex << opcode;
    }
}

class CpuDifferentialExampleTest : public ::testing::TestWithParam<const char*> {};

// The example programs, also with the sound chip attached, with joystick input
// and over several frames: the whole CPU state and the picture must be the same.
TEST_P(CpuDifferentialExampleTest, ProgramRunsLikeTheReference) {
    std::string path = std::string(EXAMPLES_DIR) + "/" + GetParam() + ".hex";
    Video videoA;
    Video videoB;
    Cpu a(&videoA);
    CpuReference b(&videoB);
    Bu8763 soundA;
    Bu8763 soundB;
    ASSERT_TRUE(FileHandler::readFile(path.c_str(), a.getROM()));
    ASSERT_TRUE(FileHandler::readFile(path.c_str(), b.ROM));
    a.setSound(&soundA);
    b.setSound(&soundB);

    const char* frames = getenv("XGSEMU_DIFFERENTIAL_FRAMES");
    const int frameCount = frames ? atoi(frames) : 10;
    for (int frame = 0; frame < frameCount; frame++) {
        int joystick = 0;
        if (frame >= 2 && frame < 4) {
            joystick = 0x10;                       // fire
        } else if (frame >= 4) {
            static const int directions[4] = {0x1, 0x8, 0x2, 0x4};
            joystick = directions[(frame / 2) % 4];
        }
        *a.getJoystrickr() = joystick;
        b.joystickr = joystick;

        a.runCycles(1333333);
        b.runCycles(1333333);

        ASSERT_EQ("", compareState(a, b)) << GetParam() << ", frame " << frame;
        ASSERT_EQ("", compareVideo(videoA, videoB)) << GetParam() << ", frame " << frame;
        ASSERT_EQ(0, memcmp(a.SRAM, b.SRAM, sizeof(a.SRAM))) << GetParam() << ", SRAM, frame " << frame;
        for (int c = 0; c < Bu8763::CHANNELS; c++) {
            ASSERT_EQ(soundB.freq(c), soundA.freq(c)) << GetParam() << ", sound channel " << c;
        }
        ASSERT_EQ(soundB.oper(), soundA.oper()) << GetParam() << ", sound";
    }
}

INSTANTIATE_TEST_SUITE_P(Examples, CpuDifferentialExampleTest,
                         ::testing::Values("kj_3d_01", "kj_pong_01", "kj_sprites_01", "mic_flags_01",
                                           "mic_plane_01", "mic_plasma_01", "mic_rzoom_01", "ms_pac_man",
                                           "ntsc_color_xme_01", "racer_01", "rem_pac_01", "rem_tetris_01",
                                           "starfield_01", "starfield_gradient_01",
                                           "starfield_gradient_color_01", "starfield_gradient_depth_01",
                                           "starfield_trails_01"));
