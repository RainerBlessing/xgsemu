#include <gtest/gtest.h>
#include "../src/DisplayLayout.h"

// The picture is shown like on a television: 4:3, centered, with black bars
// where the window has another shape.
TEST(DisplayLayoutTest, AWindowWithTheAspectRatioIsFilledCompletely) {
    DisplayRect r = fitAspect(800, 600, 4, 3);
    EXPECT_EQ(0, r.x);
    EXPECT_EQ(0, r.y);
    EXPECT_EQ(800, r.w);
    EXPECT_EQ(600, r.h);
}

TEST(DisplayLayoutTest, AWideWindowGetsBarsLeftAndRight) {
    DisplayRect r = fitAspect(1000, 600, 4, 3);
    EXPECT_EQ(100, r.x);
    EXPECT_EQ(0, r.y);
    EXPECT_EQ(800, r.w);
    EXPECT_EQ(600, r.h);
}

TEST(DisplayLayoutTest, ATallWindowGetsBarsAboveAndBelow) {
    DisplayRect r = fitAspect(800, 800, 4, 3);
    EXPECT_EQ(0, r.x);
    EXPECT_EQ(100, r.y);
    EXPECT_EQ(800, r.w);
    EXPECT_EQ(600, r.h);
}

TEST(DisplayLayoutTest, TheResultHasTheAspectRatioForAnyWindowSize) {
    for (int w = 100; w <= 2000; w += 137) {
        for (int h = 100; h <= 1200; h += 89) {
            DisplayRect r = fitAspect(w, h, 4, 3);
            // within one pixel of 4:3, and never larger than the window
            EXPECT_LE(r.w, w);
            EXPECT_LE(r.h, h);
            EXPECT_LE(abs(r.w * 3 - r.h * 4), 4) << w << "x" << h;
        }
    }
}

TEST(DisplayLayoutTest, AnEmptyWindowGivesAnEmptyRectangle) {
    DisplayRect r = fitAspect(0, 0, 4, 3);
    EXPECT_EQ(0, r.w);
    EXPECT_EQ(0, r.h);
}

TEST(DisplayLayoutTest, ANonPositiveAspectRatioGivesAnEmptyRectangle) {
    DisplayRect r = fitAspect(800, 600, 4, 0);
    EXPECT_EQ(0, r.w);
    EXPECT_EQ(0, r.h);
    r = fitAspect(800, 600, 0, 3);
    EXPECT_EQ(0, r.w);
    EXPECT_EQ(0, r.h);
}
