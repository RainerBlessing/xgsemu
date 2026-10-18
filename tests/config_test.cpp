#include <gtest/gtest.h>
#include "../src/Config.h"
#include "test_file.h"

TEST(ConfigTest, AnUnknownKeyGivesTheDefault) {
    TestFile file(".ini");
    Config config(file.name());
    EXPECT_EQ("", config.get("LoadDirectory"));
    EXPECT_EQ("here", config.get("LoadDirectory", "here"));
}

TEST(ConfigTest, AFileThatIsNotThereGivesTheDefaults) {
    Config config("no_such_directory/config.ini");
    EXPECT_EQ(".", config.get("LoadDirectory", "."));
}

TEST(ConfigTest, AValueIsRememberedByTheNextRun) {
    TestFile file(".ini");
    {
        Config config(file.name());
        config.set("LoadDirectory", "/games/sx");
    }
    Config again(file.name());
    EXPECT_EQ("/games/sx", again.get("LoadDirectory", "."));
}

TEST(ConfigTest, SettingAKeyAgainReplacesItAndKeepsTheOthers) {
    TestFile file(".ini");
    Config config(file.name());
    config.set("A", "1");
    config.set("B", "2");
    config.set("A", "3");
    Config again(file.name());
    EXPECT_EQ("3", again.get("A"));
    EXPECT_EQ("2", again.get("B"));
}

TEST(ConfigTest, AValueMayContainAnEqualSign) {
    TestFile file(".ini");
    Config config(file.name());
    config.set("path", "/a=b/c");
    Config again(file.name());
    EXPECT_EQ("/a=b/c", again.get("path"));
}

TEST(ConfigTest, ALineWithoutAnEqualSignIsIgnored) {
    TestFile file(".ini", "garbage\nkey=value\n");
    Config config(file.name());
    EXPECT_EQ("value", config.get("key"));
    EXPECT_EQ("", config.get("garbage"));
}

// Not being able to write the file is no reason to stop the emulator.
TEST(ConfigTest, AFileThatCannotBeWrittenStillKeepsTheValueForThisRun) {
    Config config("no_such_directory/config.ini");
    config.set("LoadDirectory", "/x");
    EXPECT_EQ("/x", config.get("LoadDirectory"));
}
