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

#ifndef _WIN32
#include <stdio.h>
#include <sys/stat.h>

// Where the settings live on Linux and macOS: ~/.config/xgsemu/config.ini
class ConfigInHomeTest : public ::testing::Test {
protected:
    std::string home;

    void SetUp() override {
        home = testing::TempDir() + "xgsemu_home_test";
        removeAll();
        mkdir(home.c_str(), 0755);
    }

    void TearDown() override {
        removeAll();
    }

    bool isDirectory(const std::string& path) {
        struct stat info;
        return stat(path.c_str(), &info) == 0 && S_ISDIR(info.st_mode);
    }

    void removeAll() {
        remove((home + "/.config/xgsemu/config.ini").c_str());
        rmdir((home + "/.config/xgsemu").c_str());
        rmdir((home + "/.config").c_str());
        rmdir(home.c_str());
    }
};

TEST_F(ConfigInHomeTest, TheFileIsInTheConfigDirectoryOfTheHome) {
    EXPECT_EQ(home + "/.config/xgsemu/config.ini", Config::pathInHome(home.c_str()));
}

// Without ~/.config nothing could be written: mkdir makes one directory only.
TEST_F(ConfigInHomeTest, TheDirectoriesAreCreatedWhenTheyAreNotThere) {
    Config::pathInHome(home.c_str());
    EXPECT_TRUE(isDirectory(home + "/.config"));
    EXPECT_TRUE(isDirectory(home + "/.config/xgsemu"));
}

TEST_F(ConfigInHomeTest, ASettingIsWrittenAndFoundAgain) {
    std::string path = Config::pathInHome(home.c_str());
    {
        Config config(path);
        config.set("LoadDirectory", "/games");
    }
    EXPECT_EQ("/games", Config(Config::pathInHome(home.c_str())).get("LoadDirectory"));
}

TEST_F(ConfigInHomeTest, ExistingDirectoriesAreNoProblem) {
    Config::pathInHome(home.c_str());
    EXPECT_EQ(home + "/.config/xgsemu/config.ini", Config::pathInHome(home.c_str()));
}

TEST(ConfigWithoutHomeTest, TheFileIsInTheCurrentDirectory) {
    EXPECT_EQ("./config.ini", Config::pathInHome(NULL));
}
#endif
