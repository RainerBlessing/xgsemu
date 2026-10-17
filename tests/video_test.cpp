#include <gtest/gtest.h>
#include <string.h>
#include "../src/Video.h"

// Drives Video with a synthetic signal: vsync, hsync, colour burst and one
// long white fragment. The fragments are kept in a ring buffer of 1000
// entries, so the line is placed such that the ring buffer wraps around
// right between the hsync and the colour burst.
class VideoTest : public ::testing::Test {
protected:
    Video video;
    int cycle = 0;

    static const int SYNC = 0x00;
    static const int BLANK = 0x07;
    static const int WHITE = 0xff;
    static const int RING_SIZE = 1000;

    static void signal(Video& target, int& clock, int duration, int colour) {
        clock += duration;
        target.changeVideo(clock, colour);
    }

    // A long sync pulse (vsync) is followed by blank level. After it the ring
    // buffer index is 1. The line sync pulse is placed on index RING_SIZE - 1,
    // so the colour burst is seen with index 0 and the visible fragment with 1.
    static void drawLongLineAcrossRingBufferWrap(Video& target, int& clock) {
        signal(target, clock, 0, SYNC);
        signal(target, clock, 5000, BLANK);  // vsync
        for (int i = 1; i < RING_SIZE - 2; i++) {
            signal(target, clock, 1, BLANK);
        }
        signal(target, clock, 1, SYNC);      // the next index (RING_SIZE - 1) is a sync pulse
        signal(target, clock, 400, BLANK);   // hsync, the index wraps to 0 afterwards
        signal(target, clock, 45, WHITE);    // colour burst, previous fragment is index 999
        signal(target, clock, 4600, SYNC);   // visible fragment reaching beyond the right edge
    }

    // Only every second frame is drawn, so send two frames.
    static void drawTwoFrames(Video& target, int& clock) {
        drawLongLineAcrossRingBufferWrap(target, clock);
        drawLongLineAcrossRingBufferWrap(target, clock);
    }

    void drawTwoFrames() {
        drawTwoFrames(video, cycle);
    }

    static Uint32 pixel(Video& target, int x, int y) {
        SDL_Surface* screen = target.getScreen();
        Uint8* row = (Uint8*)screen->pixels + y * screen->pitch;
        return ((Uint32*)row)[x];
    }

    Uint32 pixel(int x, int y) {
        return pixel(video, x, y);
    }

    static int lastRowReachingTheRightEdge(Video& target) {
        SDL_Surface* screen = target.getScreen();
        for (int y = screen->h - 1; y >= 0; y--) {
            if (pixel(target, screen->w - 1, y) != 0) {
                return y;
            }
        }
        return -1;
    }

    int lastRowReachingTheRightEdge() {
        return lastRowReachingTheRightEdge(video);
    }

    static int lastDrawnRow(Video& target) {
        SDL_Surface* screen = target.getScreen();
        for (int y = screen->h - 1; y >= 0; y--) {
            for (int x = 0; x < screen->w; x++) {
                if (pixel(target, x, y) != 0) {
                    return y;
                }
            }
        }
        return -1;
    }
};

TEST_F(VideoTest, ColourBurstIsDetectedWhenTheRingBufferWraps) {
    drawTwoFrames();
    bool drawn = false;
    for (int y = 0; y < 20 && !drawn; y++) {
        drawn = pixel(100, y) != 0;
    }
    EXPECT_TRUE(drawn);
}

TEST_F(VideoTest, LineReachesTheLastColumn) {
    drawTwoFrames();
    EXPECT_GE(lastRowReachingTheRightEdge(), 0);
}

TEST_F(VideoTest, LineDoesNotWrapIntoTheNextRow) {
    drawTwoFrames();
    int row = lastRowReachingTheRightEdge();
    ASSERT_GE(row, 0);
    // A pixel written past the end of the row lands in the first column of
    // the row below.
    EXPECT_EQ(0u, pixel(0, row + 1));
}

// The cycle counter of the CPU runs over after about 27 seconds. The line
// timing only uses differences, so it must not matter where the counter is.
TEST_F(VideoTest, LineIsDrawnWhenTheCycleCounterRunsOver) {
    cycle = -3000;   // the counter wraps to positive values within the line
    drawTwoFrames();
    EXPECT_GE(lastRowReachingTheRightEdge(), 0);
}

TEST_F(VideoTest, ASecondVideoObjectStartsWithItsOwnState) {
    drawTwoFrames();
    Video second;
    int secondCycle = 0;
    drawTwoFrames(second, secondCycle);

    SDL_Surface* a = video.getScreen();
    SDL_Surface* b = second.getScreen();
    EXPECT_EQ(0, memcmp(a->pixels, b->pixels, a->pitch * a->h));
}

TEST_F(VideoTest, ResetForgetsTheTimingOfTheOldSignal) {
    drawTwoFrames();
    video.reset();
    memset(video.getScreen()->pixels, 0, video.getScreen()->pitch * video.getScreen()->h);
    // A new program starts counting cycles at 0 again. A stale timestamp
    // from before the reset must not swallow the first vsync.
    cycle = 0;
    drawTwoFrames();
    EXPECT_GE(lastRowReachingTheRightEdge(), 0);
}

// Every 101st frame was drawn twice, because the frame counter wrapped from
// 100 to 0 and both are even.
TEST_F(VideoTest, EveryOtherFrameIsDrawnAlsoAcrossTheFrameCounterWrap) {
    int drawn = 0;
    SDL_Surface* screen = video.getScreen();
    for (int frame = 0; frame < 202; frame++) {
        memset(screen->pixels, 0, screen->pitch * screen->h);
        drawLongLineAcrossRingBufferWrap(video, cycle);
        if (lastRowReachingTheRightEdge() >= 0) {
            drawn++;
        }
    }
    EXPECT_EQ(101, drawn);
}

// What a picture fragment looks like. A line of the signal starts when the sync
// pulse ends (hsync). The fragments are measured from there in units of 8 cycles,
// and the visible part (50..560) is stretched by 1.1 to the width of the picture.
class VideoPictureTest : public VideoTest {
protected:
    // frame parity: only every second frame is drawn, so start a few of them
    void startFrames() {
        signal(video, cycle, 0, SYNC);
        signal(video, cycle, 5000, BLANK);
    }

    // One line: hsync, colour burst, then the fragments. "colours" are the colours of
    // the fragments, "durations" how long each one lasts.
    void line(const std::vector<int>& colours, const std::vector<int>& durations) {
        signal(video, cycle, 20, SYNC);
        signal(video, cycle, 400, BLANK);                        // the sync pulse ends: hsync
        signal(video, cycle, 45, colours[0]);                    // colour burst
        for (size_t i = 0; i < durations.size(); i++) {
            signal(video, cycle, durations[i], i + 1 < colours.size() ? colours[i + 1] : SYNC);
        }
    }

    // Only every second frame is drawn: the first frame has just the vsync, the line is in the
    // second one. (A line of the first frame would leave 'line' set, and the first call after the
    // next vsync would draw a stray fragment.)
    void drawTwice(const std::vector<int>& colours, const std::vector<int>& durations) {
        startFrames();
        startFrames();
        line(colours, durations);
    }

    int firstDrawnRow() {
        SDL_Surface* screen = video.getScreen();
        for (int y = 0; y < screen->h; y++) {
            for (int x = 0; x < screen->w; x++) {
                if (pixel(x, y) != 0) {
                    return y;
                }
            }
        }
        return -1;
    }

    void rgb(Uint32 pixelValue, int* r, int* g, int* b) {
        Uint8 red, green, blue;
        SDL_GetRGB(pixelValue, video.getScreen()->format, &red, &green, &blue);
        *r = red;
        *g = green;
        *b = blue;
    }
};

TEST_F(VideoPictureTest, AFragmentIsDrawnInTwoRows) {
    drawTwice({0xff}, {4400});
    int row = firstDrawnRow();
    ASSERT_GE(row, 0);
    EXPECT_NE(0u, pixel(100, row));
    EXPECT_EQ(pixel(100, row), pixel(100, row + 1));
    EXPECT_EQ(0u, pixel(100, row + 2));
}

// Chroma 15 is grey: the luminance 6..15 gives the level 26..255, below it is black.
TEST_F(VideoPictureTest, GreyFragmentsFollowTheLuminance) {
    static const int levels[] = {0, 0, 0, 0, 0, 0, 26, 51, 77, 102, 128, 153, 179, 204, 230, 255};
    for (int luminance = 6; luminance < 16; luminance++) {
        SCOPED_TRACE(luminance);
        Video fresh;
        int clock = 0;
        for (int pass = 0; pass < 2; pass++) {       // the second frame is the drawn one
            signal(fresh, clock, 0, SYNC);
            signal(fresh, clock, 5000, BLANK);
        }
        signal(fresh, clock, 20, SYNC);
        signal(fresh, clock, 400, BLANK);
        signal(fresh, clock, 45, 0xf0 | luminance);
        signal(fresh, clock, 4400, SYNC);
        int row = lastDrawnRow(fresh);
        ASSERT_GE(row, 0);
        Uint8 red, green, blue;
        SDL_GetRGB(pixel(fresh, 100, row), fresh.getScreen()->format, &red, &green, &blue);
        EXPECT_EQ(levels[luminance], red);
        EXPECT_EQ(red, green);
        EXPECT_EQ(green, blue);
    }
}

TEST_F(VideoPictureTest, PositionsAreScaledToTheWidthOfThePicture) {
    // fragment 1: columns 0..55 (positions 50..100), fragment 2: 55..165 (positions 100..200)
    drawTwice({0xf8, 0xfc}, {755, 800});
    int row = firstDrawnRow();
    ASSERT_GE(row, 0);
    int r, g, b;
    rgb(pixel(0, row), &r, &g, &b);
    EXPECT_EQ(77, r);
    rgb(pixel(54, row), &r, &g, &b);
    EXPECT_EQ(77, r);
    rgb(pixel(55, row), &r, &g, &b);
    EXPECT_EQ(179, r);
    rgb(pixel(164, row), &r, &g, &b);
    EXPECT_EQ(179, r);
    EXPECT_EQ(0u, pixel(165, row));
}

TEST_F(VideoPictureTest, ThePartBeforeThePositionFiftyIsNotVisible) {
    // a fragment that ends at position 40 is outside of the picture
    drawTwice({0xff}, {40 * 8 - 45});
    EXPECT_EQ(-1, firstDrawnRow());
}

TEST_F(VideoPictureTest, TheLineCounterStopsAtTheLowerEdge) {
    // 400 lines without a vsync: the picture has 525 rows, the line counter stops at 520
    startFrames();
    startFrames();
    for (int i = 0; i < 400; i++) {
        line({0xff}, {4400});
    }
    SDL_Surface* screen = video.getScreen();
    for (int y = 522; y < screen->h; y++) {
        EXPECT_EQ(0u, pixel(100, y)) << "row " << y;
    }
    EXPECT_NE(0u, pixel(100, 520));
    EXPECT_NE(0u, pixel(100, 521));
}

// A sync pulse (luminance 0) longer than 360 cycles starts a line, one longer than
// 4000 cycles starts a frame, and the colour burst is a fragment of 41..49 cycles.
class VideoThresholdTest : public VideoPictureTest {
protected:
    // frame 1 has the vsync only, frame 2 is drawn: a sync pulse of "syncCycles"
    // after the vsync, then the burst of "burstCycles" and a picture fragment
    void lineAfterTheSyncPulse(int syncCycles, int burstCycles) {
        startFrames();
        startFrames();
        signal(video, cycle, 20, SYNC);
        signal(video, cycle, syncCycles, BLANK);
        signal(video, cycle, burstCycles, 0xff);
        signal(video, cycle, 4400, SYNC);
    }
};

TEST_F(VideoThresholdTest, ASyncPulseOf360CyclesIsNoNewLine) {
    lineAfterTheSyncPulse(360, 45);
    line({0xff}, {4400});                // a real line
    EXPECT_NE(0u, pixel(100, 2));        // it is the first line of the frame
    EXPECT_EQ(0u, pixel(100, 4));
}

TEST_F(VideoThresholdTest, ASyncPulseOf361CyclesStartsALine) {
    lineAfterTheSyncPulse(361, 45);
    line({0xff}, {4400});
    EXPECT_NE(0u, pixel(100, 2));
    EXPECT_NE(0u, pixel(100, 4));        // and the real line is the second one
}

TEST_F(VideoThresholdTest, AColourBurstOf49CyclesIsRecognised) {
    lineAfterTheSyncPulse(400, 49);
    EXPECT_EQ(2, firstDrawnRow());
}

TEST_F(VideoThresholdTest, AFragmentOf50CyclesIsNoColourBurst) {
    lineAfterTheSyncPulse(400, 50);
    EXPECT_EQ(-1, firstDrawnRow());      // without the burst nothing of the line is drawn
}

TEST_F(VideoThresholdTest, AFragmentOf40CyclesIsNoColourBurst) {
    lineAfterTheSyncPulse(400, 40);
    EXPECT_EQ(-1, firstDrawnRow());
}

TEST_F(VideoThresholdTest, ASyncPulseOf4000CyclesIsStillALine) {
    startFrames();
    startFrames();
    line({0xff}, {4400});                // row 2
    signal(video, cycle, 20, SYNC);
    signal(video, cycle, 4000, BLANK);
    signal(video, cycle, 45, 0xff);
    signal(video, cycle, 4400, SYNC);
    EXPECT_NE(0u, pixel(100, 4));        // the next line
}

TEST_F(VideoThresholdTest, ASyncPulseOf4001CyclesStartsAFrame) {
    startFrames();
    startFrames();
    line({0xff}, {4400});                // row 2
    signal(video, cycle, 20, SYNC);
    signal(video, cycle, 4001, BLANK);   // the next frame is not drawn
    signal(video, cycle, 45, 0xff);
    signal(video, cycle, 4400, SYNC);
    EXPECT_EQ(0u, pixel(100, 4));
    EXPECT_NE(0u, pixel(100, 2));
}

// The phase of a colour is the chroma plus the chroma of the colour burst (as the
// code takes it), modulo 28 colours of the colour wheel.
TEST_F(VideoPictureTest, ColoursFollowTheChromaAndThePhaseOfTheBurst) {
    struct Expectation {
        int chroma;
        int red, green, blue;
    };
    // the colour wheel at index 2 * chroma, at full luminance: base colour * 255 / 256
    static const Expectation expected[] = {
        {1, 0, 127, 254},      // index 2: (0, 128, 255)
        {4, 254, 0, 254},      // index 8: (255, 0, 255)
        {7, 254, 254, 0},      // index 14: (255, 255, 0)
        {13, 0, 254, 169},     // index 26: (0, 255, 170)
        {14, 0, 254, 254},     // index 28 wraps to 0: (0, 255, 255)
    };
    for (const Expectation& e : expected) {
        SCOPED_TRACE(e.chroma);
        Video fresh;
        int clock = 0;
        for (int pass = 0; pass < 2; pass++) {
            signal(fresh, clock, 0, SYNC);
            signal(fresh, clock, 5000, BLANK);
        }
        signal(fresh, clock, 20, SYNC);
        signal(fresh, clock, 400, BLANK);
        signal(fresh, clock, 45, (e.chroma << 4) | 15);
        signal(fresh, clock, 4400, SYNC);
        int row = lastDrawnRow(fresh);
        ASSERT_GE(row, 0);
        Uint8 red, green, blue;
        SDL_GetRGB(pixel(fresh, 100, row), fresh.getScreen()->format, &red, &green, &blue);
        EXPECT_EQ(e.red, red);
        EXPECT_EQ(e.green, green);
        EXPECT_EQ(e.blue, blue);
    }
}
