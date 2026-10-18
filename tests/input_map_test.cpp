#include <gtest/gtest.h>
#include "../src/InputMap.h"

TEST(InputMapTest, TheArrowKeysAreTheDirections) {
    EXPECT_EQ(0x01, joystickMaskForKey(SDLK_UP));
    EXPECT_EQ(0x02, joystickMaskForKey(SDLK_DOWN));
    EXPECT_EQ(0x04, joystickMaskForKey(SDLK_LEFT));
    EXPECT_EQ(0x08, joystickMaskForKey(SDLK_RIGHT));
}

TEST(InputMapTest, ControlAndSpaceAreTheFireButton) {
    EXPECT_EQ(0x10, joystickMaskForKey(SDLK_LCTRL));
    EXPECT_EQ(0x10, joystickMaskForKey(SDLK_RCTRL));
    EXPECT_EQ(0x10, joystickMaskForKey(SDLK_SPACE));
}

TEST(InputMapTest, OtherKeysAreNoButtons) {
    EXPECT_EQ(0, joystickMaskForKey(SDLK_a));
    EXPECT_EQ(0, joystickMaskForKey(SDLK_ESCAPE));
    EXPECT_EQ(0, joystickMaskForKey(SDLK_F5));
}
