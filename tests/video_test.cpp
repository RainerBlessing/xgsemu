#include <gtest/gtest.h>
#include <string.h>
#include <algorithm>
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

    static uint32_t pixel(Video& target, int x, int y) {
        return target.pixels()[y * target.width() + x];
    }

    // red, green and blue of a drawn pixel
    static void unpack(uint32_t value, int* red, int* green, int* blue) {
        *red = (value >> 16) & 0xff;
        *green = (value >> 8) & 0xff;
        *blue = value & 0xff;
    }

    uint32_t pixel(int x, int y) {
        return pixel(video, x, y);
    }

    static int lastRowReachingTheRightEdge(Video& target) {
        for (int y = target.height() - 1; y >= 0; y--) {
            if (pixel(target, target.width() - 1, y) != 0) {
                return y;
            }
        }
        return -1;
    }

    int lastRowReachingTheRightEdge() {
        return lastRowReachingTheRightEdge(video);
    }

    static int lastDrawnRow(Video& target) {
        for (int y = target.height() - 1; y >= 0; y--) {
            for (int x = 0; x < target.width(); x++) {
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

    EXPECT_EQ(0, memcmp(video.pixels(), second.pixels(), video.pitch() * video.height()));
}

TEST_F(VideoTest, ResetForgetsTheTimingOfTheOldSignal) {
    drawTwoFrames();
    video.reset();
    memset(video.pixels(), 0, video.pitch() * video.height());
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
    uint32_t* screen = video.pixels();
    for (int frame = 0; frame < 202; frame++) {
        memset(screen, 0, video.pitch() * video.height());
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
        for (int y = 0; y < video.height(); y++) {
            for (int x = 0; x < video.width(); x++) {
                if (pixel(x, y) != 0) {
                    return y;
                }
            }
        }
        return -1;
    }

    void rgb(uint32_t pixelValue, int* r, int* g, int* b) {
        unpack(pixelValue, r, g, b);
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
        int red, green, blue;
        unpack(pixel(fresh, 100, row), &red, &green, &blue);
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
    for (int y = 522; y < video.height(); y++) {
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

// The colour of a fragment: the phase of its chroma against the colour burst on a wheel
// of 32 steps of 11.25 degrees (chroma minus burst), at a brightness given by the
// luminance: every channel is between a lowest and a highest level that belong to the
// luminance. The reference are the cells of the colour chart in the documentation of
// Rem-Pac (burst 0, chroma 0..14, luminance 5..14), which this model reproduces with
// an error of less than 2 of 255, and the photos of figure 11.33 of the book.
class VideoColourTest : public VideoPictureTest {
protected:
    struct Rgb {
        int red, green, blue;
    };

    // The colour of a picture fragment in a line whose colour burst has the chroma "burst".
    Rgb colourOf(int chroma, int burst, int luminance) {
        Video fresh;
        int clock = 0;
        for (int pass = 0; pass < 2; pass++) {
            signal(fresh, clock, 0, SYNC);
            signal(fresh, clock, 5000, BLANK);
        }
        signal(fresh, clock, 20, SYNC);
        signal(fresh, clock, 400, BLANK);
        signal(fresh, clock, 45, (burst << 4) | 5);                   // the colour burst follows the breezeway
        signal(fresh, clock, 200, (chroma << 4) | luminance);         // the picture
        signal(fresh, clock, 4000, SYNC);
        int row = lastDrawnRow(fresh);
        Rgb colour = {-1, -1, -1};
        if (row >= 0) {
            int red, green, blue;
            unpack(pixel(fresh, 100, row), &red, &green, &blue);
            colour.red = red;
            colour.green = green;
            colour.blue = blue;
        }
        return colour;
    }

    // a cell of the colour chart, burst 0
    void expectChartCell(int chroma, int luminance, int red, int green, int blue) {
        SCOPED_TRACE(::testing::Message() << "chroma " << chroma << ", luminance " << luminance);
        Rgb colour = colourOf(chroma, 0, luminance);
        EXPECT_NEAR(red, colour.red, 3);
        EXPECT_NEAR(green, colour.green, 3);
        EXPECT_NEAR(blue, colour.blue, 3);
    }
};

// Chart cells at a low luminance: here the colours are much brighter than the hue scaled with
// the luminance, the chroma is still there at the black level.
TEST_F(VideoColourTest, ColoursMatchTheColourChartAtLowLuminance) {
    expectChartCell(0, 5, 74, 74, 14);        // dark olive
    expectChartCell(8, 5, 75, 14, 47);
    expectChartCell(14, 5, 36, 14, 76);
    expectChartCell(0, 7, 187, 182, 38);      // olive yellow
    expectChartCell(5, 7, 186, 43, 37);       // red
    expectChartCell(10, 7, 187, 37, 170);     // magenta
    expectChartCell(14, 7, 90, 37, 187);      // the maze of the first level of Rem-Pac, "light blueish"
}

// At a higher luminance the colours stay saturated: the photo of Racer City on the real
// hardware has an orange sky (225, 49, 4), a yellow-green meadow (205, 215, 71) and orange
// kerbs (226, 65, 10); the capture of the plane demo has a ratio of 0.24 of the lowest to the
// highest channel at luminance 15. (The colour chart of Rem-Pac goes to 0.65 there.)
TEST_F(VideoColourTest, ColoursStaySaturatedAtHighLuminance) {
    for (int luminance = 9; luminance <= 15; luminance++) {
        for (int chroma = 0; chroma < 15; chroma++) {
            Rgb colour = colourOf(chroma, 0, luminance);
            int highest = std::max(colour.red, std::max(colour.green, colour.blue));
            int lowest = std::min(colour.red, std::min(colour.green, colour.blue));
            EXPECT_LE(lowest * 100, highest * 32) << "chroma " << chroma << ", luminance " << luminance;
            EXPECT_GE(highest, 230) << "chroma " << chroma << ", luminance " << luminance;
        }
    }
}

TEST_F(VideoColourTest, TheBrightestLuminanceIsNoWhiteForAColour) {
    Rgb colour = colourOf(5, 0, 15);               // red
    EXPECT_EQ(255, colour.red);
    EXPECT_LT(colour.green, 100);
    EXPECT_LT(colour.blue, 100);
}

TEST_F(VideoColourTest, ARaisedLuminanceMakesTheSameHueBrighter) {
    int last = -1;
    for (int luminance = 5; luminance < 15; luminance++) {
        Rgb colour = colourOf(5, 0, luminance);     // red
        EXPECT_GE(colour.red, last) << luminance;
        last = colour.red;
    }
}

TEST_F(VideoColourTest, BelowTheBlackLevelEveryColourIsBlack) {
    for (int luminance = 1; luminance < 5; luminance++) {
        Rgb colour = colourOf(5, 0, luminance);
        EXPECT_EQ(0, colour.red) << luminance;
        EXPECT_EQ(0, colour.green) << luminance;
        EXPECT_EQ(0, colour.blue) << luminance;
    }
}

// With burst 14 the chroma 0..14 are the other half of the circle, from blue through
// green to yellow (photo, figure 11.33(b)).
TEST_F(VideoColourTest, WithBurstFourteenTheChromaGoesFromBlueThroughGreenToYellow) {
    Rgb blue = colourOf(0, 14, 9);
    EXPECT_GT(blue.blue, blue.green);
    EXPECT_GT(blue.blue, blue.red);
    Rgb green = colourOf(7, 14, 9);
    EXPECT_GT(green.green, green.red);
    EXPECT_GT(green.green, green.blue);
    Rgb yellow = colourOf(14, 14, 9);
    Rgb reference = colourOf(0, 0, 9);            // the same phase: chroma 0 with burst 0
    EXPECT_EQ(reference.red, yellow.red);
    EXPECT_EQ(reference.green, yellow.green);
    EXPECT_EQ(reference.blue, yellow.blue);
    EXPECT_GT(yellow.red, yellow.blue);
}

TEST_F(VideoColourTest, OnlyTheDifferenceToTheBurstCounts) {
    Rgb a = colourOf(3, 1, 9);
    Rgb b = colourOf(5, 3, 9);
    Rgb c = colourOf(14, 12, 9);
    EXPECT_EQ(a.red, b.red);
    EXPECT_EQ(a.green, b.green);
    EXPECT_EQ(a.blue, b.blue);
    EXPECT_EQ(a.red, c.red);
    EXPECT_EQ(a.green, c.green);
    EXPECT_EQ(a.blue, c.blue);
}

TEST_F(VideoColourTest, TheBurstIsSubtractedNotAdded) {
    Rgb a = colourOf(3, 1, 9);   // difference +2
    Rgb b = colourOf(1, 3, 9);   // difference -2, as much further the other way
    EXPECT_FALSE(a.red == b.red && a.green == b.green && a.blue == b.blue);
}

TEST_F(VideoColourTest, ANegativeDifferenceWrapsAroundTheWheel) {
    // 15 steps back from yellow: hue 59 + 168.75 = 227.75 degrees, between cyan and blue
    Rgb colour = colourOf(0, 15, 5);      // luminance 5: the levels of the chart, 14 and 75
    EXPECT_NEAR(14, colour.red, 3);
    EXPECT_NEAR(14 + 61 * 52 / 255, colour.green, 3);
    EXPECT_NEAR(75, colour.blue, 3);
}

TEST_F(VideoColourTest, ChromaFifteenIsGreyWhateverTheBurst) {
    for (int burst = 0; burst < 16; burst++) {
        SCOPED_TRACE(burst);
        Rgb colour = colourOf(15, burst, 15);
        EXPECT_EQ(255, colour.red);     // grey is the luminance itself
        EXPECT_EQ(colour.red, colour.green);
        EXPECT_EQ(colour.green, colour.blue);
    }
}
