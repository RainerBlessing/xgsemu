#include <gtest/gtest.h>
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

    void signal(int duration, int colour) {
        cycle += duration;
        video.changeVideo(cycle, colour);
    }

    // After the vsync the ring buffer index is 1. The hsync is placed on
    // index RING_SIZE - 1, so the colour burst is seen with index 0.
    void drawLongLineAcrossRingBufferWrap() {
        signal(0, SYNC);
        signal(5000, SYNC);   // vsync
        signal(400, BLANK);   // hsync, index 1
        for (int i = 2; i < RING_SIZE - 1; i++) {
            signal(1, BLANK);
        }
        signal(1, SYNC);      // index RING_SIZE - 1 is a sync pulse
        signal(400, BLANK);   // hsync, the index wraps to 0
        signal(45, WHITE);    // colour burst, previous fragment is index 999
        signal(4600, SYNC);   // visible fragment reaching beyond the right edge
    }

    Uint32 pixel(int x, int y) {
        SDL_Surface* screen = video.getScreen();
        Uint8* row = (Uint8*)screen->pixels + y * screen->pitch;
        return ((Uint32*)row)[x];
    }

    int lastRowReachingTheRightEdge() {
        SDL_Surface* screen = video.getScreen();
        for (int y = screen->h - 1; y >= 0; y--) {
            if (pixel(screen->w - 1, y) != 0) {
                return y;
            }
        }
        return -1;
    }

    // Only every second frame is drawn, so send two frames.
    void drawTwoFrames() {
        drawLongLineAcrossRingBufferWrap();
        drawLongLineAcrossRingBufferWrap();
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
