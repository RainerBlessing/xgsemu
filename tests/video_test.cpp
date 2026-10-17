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
