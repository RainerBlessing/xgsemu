// Differential test for the refactoring of Video::changeVideo.
//
// VideoReference is a frozen copy of the Video from before the refactoring.
// Both get the same signal and must end up in the same state with the same
// picture. The signals are random but built like a video signal (sync pulses,
// colour burst, picture fragments, many values right at the thresholds), and
// the signals that the example programs produce.
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
#include "../src/Video.h"
#include "../src/cpu.h"
#include "video_reference.h"
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

    int between(int low, int high) {
        return low + (int)below(high - low + 1);
    }

private:
    uint64_t state;
};

struct Call {
    int cycle;
    unsigned char colour;
};

// Logs what the CPU sends to the video.
class RecordingVideo : public Video {
public:
    void changeVideo(int cycle, unsigned char colour) override {
        Call call = {cycle, colour};
        calls.push_back(call);
    }

    std::vector<Call> calls;
};

unsigned char pictureColour(Rng& r) {
    int chroma = r.below(16);
    int luminance = 1 + r.below(15);
    if (r.below(12) == 0) {
        luminance = 0;                 // looks like a sync pulse in the middle of the picture
    }
    return (chroma << 4) | luminance;
}

// Durations that sit right at the thresholds of the line detection.
int edgeDuration(Rng& r) {
    static const int edges[] = {0, 1, 2, 7, 8, 9, 39, 40, 41, 49, 50, 51, 359, 360, 361, 399, 4000, 4001, 3999, 4800};
    return edges[r.below(sizeof(edges) / sizeof(edges[0]))];
}

// A signal made of lines: sync pulse, blank, colour burst, picture fragments.
std::vector<Call> makeSignal(uint64_t seed, int lines, bool wrapTheCounter) {
    Rng r(seed);
    std::vector<Call> calls;
    int cycle = wrapTheCounter ? (int)0x7fffff00 : (int)r.next() % 100000;
    auto add = [&](int duration, unsigned char colour) {
        cycle = (int)((unsigned int)cycle + (unsigned int)duration);
        Call call = {cycle, colour};
        calls.push_back(call);
    };
    unsigned char sync = 0x00;
    for (int line = 0; line < lines; line++) {
        bool vsync = r.below(300) == 0;
        add(r.below(5) == 0 ? edgeDuration(r) : r.between(20, 60), sync);
        add(vsync ? r.between(4001, 5200) : (r.below(5) == 0 ? edgeDuration(r) : r.between(300, 420)),
            0x05 + r.below(3));
        add(r.below(4) == 0 ? edgeDuration(r) : r.between(41, 49), pictureColour(r));
        int fragments = r.below(10) == 0 ? r.between(900, 1200) : r.between(0, 40);   // long lines wrap the ring buffer
        for (int i = 0; i < fragments; i++) {
            add(r.below(6) == 0 ? edgeDuration(r) : r.between(8, 400), pictureColour(r));
        }
    }
    return calls;
}

template <class A, class B>
std::string compareState(const A& a, const B& b) {
    std::ostringstream out;
    if (a.fragmentcounter != b.fragmentcounter) out << "fragmentcounter " << a.fragmentcounter << " but expected " << b.fragmentcounter << "; ";
    if (a.yc != b.yc) out << "yc; ";
    if (a.frame != b.frame) out << "frame; ";
    if (a.vsync != b.vsync) out << "vsync; ";
    if (a.line != b.line) out << "line; ";
    if (a.x0 != b.x0) out << "x0 " << a.x0 << " but expected " << b.x0 << "; ";
    if (a.x != b.x) out << "x " << a.x << " but expected " << b.x << "; ";
    if (a.y != b.y) out << "y " << a.y << " but expected " << b.y << "; ";
    if (a.burst != b.burst) out << "burst; ";
    if (memcmp(a.slfragments, b.slfragments, sizeof(a.slfragments)) != 0) out << "fragments; ";
    return out.str();
}

template <class A, class B>
bool samePicture(const A& a, const B& b) {
    return memcmp(a.screen->pixels, b.screen->pixels, a.screen->pitch * a.screen->h) == 0;
}

// Plays the signal into both and compares after every call, the picture every
// few hundred calls and at the end.
void replay(const std::vector<Call>& calls, const std::string& name) {
    Video video;
    VideoReference reference;
    for (size_t i = 0; i < calls.size(); i++) {
        video.changeVideo(calls[i].cycle, calls[i].colour);
        reference.changeVideo(calls[i].cycle, calls[i].colour);
        std::string difference = compareState(video, reference);
        ASSERT_EQ("", difference) << name << ", call " << i;
        if (i % 512 == 0) {
            ASSERT_TRUE(samePicture(video, reference)) << name << ", picture after call " << i;
        }
    }
    ASSERT_TRUE(samePicture(video, reference)) << name << ", final picture";
}

}  // namespace

TEST(VideoDifferentialTest, RandomLinesGiveTheSameStateAndPicture) {
    for (int seed = 1; seed <= 24; seed++) {
        std::ostringstream name;
        name << "seed " << seed;
        replay(makeSignal(seed, 700, false), name.str());
        if (::testing::Test::HasFatalFailure()) {
            return;
        }
    }
}

TEST(VideoDifferentialTest, TheCycleCounterMayRunOverInTheMiddleOfTheSignal) {
    for (int seed = 100; seed < 104; seed++) {
        std::ostringstream name;
        name << "wrapping seed " << seed;
        replay(makeSignal(seed, 700, true), name.str());
        if (::testing::Test::HasFatalFailure()) {
            return;
        }
    }
}

TEST(VideoDifferentialTest, LongSignalsReachTheLowerEdgeOfThePicture) {
    // no vsync for a long time: the line counter stops at its limit
    Rng r(7);
    std::vector<Call> calls;
    int cycle = 0;
    for (int line = 0; line < 400; line++) {
        cycle += 30;  calls.push_back({cycle, 0x00});
        cycle += 400; calls.push_back({cycle, 0x06});
        cycle += 45;  calls.push_back({cycle, pictureColour(r)});
        for (int i = 0; i < 5; i++) {
            cycle += r.between(50, 700);
            calls.push_back({cycle, pictureColour(r)});
        }
    }
    // a vsync at the very beginning, otherwise nothing is drawn
    calls.insert(calls.begin(), Call{5000, 0x06});
    calls.insert(calls.begin(), Call{0, 0x00});
    replay(calls, "long signal");
}

// Lines that start or end right at the wrap of the ring buffer (index 999 to 0).
// A helper Video tells where in the ring buffer the signal is.
TEST(VideoDifferentialTest, LinesAtTheWrapOfTheRingBuffer) {
    static const int starts[] = {0, 1, 2, 3, 996, 997, 998, 999};
    static const int lengths[] = {5, 997, 998, 999, 1000, 1001, 1002};
    for (size_t a = 0; a < sizeof(starts) / sizeof(starts[0]); a++) {
        for (size_t b = 0; b < sizeof(lengths) / sizeof(lengths[0]); b++) {
            Video probe;
            std::vector<Call> calls;
            int cycle = 0;
            Rng r(starts[a] * 31 + lengths[b]);
            auto add = [&](int duration, unsigned char colour) {
                cycle += duration;
                Call call = {cycle, colour};
                calls.push_back(call);
                probe.changeVideo(cycle, colour);
            };
            for (int pass = 0; pass < 2; pass++) {      // only every second frame is drawn
                add(0, 0x00);
                add(5000, 0x06);                         // vsync
                // blank fragments until the next call is at the index in front of the line start
                while (probe.fragmentcounter != (starts[a] + 999) % 1000) {
                    add(1, 0x06);
                }
                add(1, 0x00);                            // the next fragment is a sync pulse
                add(400, 0x06);                          // hsync, the line starts at starts[a]
                add(45, pictureColour(r));               // colour burst
                for (int i = 0; i < lengths[b]; i++) {
                    add(r.between(8, 40), pictureColour(r));
                }
                add(30, 0x00);
            }
            std::ostringstream name;
            name << "line start " << starts[a] << ", " << lengths[b] << " fragments";
            replay(calls, name.str());
            if (::testing::Test::HasFatalFailure()) {
                return;
            }
        }
    }
}

class VideoDifferentialExampleTest : public ::testing::TestWithParam<const char*> {};

TEST_P(VideoDifferentialExampleTest, TheSignalOfTheProgramGivesTheSamePicture) {
    std::string path = std::string(EXAMPLES_DIR) + "/" + GetParam() + ".hex";
    RecordingVideo recording;
    Cpu cpu(&recording);
    ASSERT_TRUE(FileHandler::readFile(path.c_str(), cpu.getROM()));
    const char* frames = getenv("XGSEMU_DIFFERENTIAL_FRAMES");
    int frameCount = frames ? atoi(frames) : 4;
    for (int frame = 0; frame < frameCount; frame++) {
        cpu.runCycles(1333333);
    }
    ASSERT_GT(recording.calls.size(), 100u) << "the program did not produce a video signal";
    replay(recording.calls, GetParam());
}

INSTANTIATE_TEST_SUITE_P(Examples, VideoDifferentialExampleTest,
                         ::testing::Values("kj_3d_01", "kj_pong_01", "kj_sprites_01", "mic_flags_01",
                                           "mic_plane_01", "mic_plasma_01", "mic_rzoom_01", "ms_pac_man",
                                           "ntsc_color_xme_01", "racer_01", "rem_pac_01", "rem_tetris_01",
                                           "starfield_01", "starfield_gradient_01",
                                           "starfield_gradient_color_01", "starfield_gradient_depth_01",
                                           "starfield_trails_01"));
