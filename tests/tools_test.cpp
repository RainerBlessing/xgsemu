// sxasm and sxdis are run as programs, the way a user runs them.
#ifndef _WIN32
#include <gtest/gtest.h>
#include <stdio.h>
#include <sys/wait.h>
#include <fstream>
#include <sstream>
#include <string>

#ifndef EXAMPLES_DIR
#define EXAMPLES_DIR "examples"
#endif

namespace {

struct Result {
    int exitCode;
    std::string output;     // stdout and stderr
};

Result run(const std::string& command) {
    Result result = { -1, "" };
    FILE* pipe = popen((command + " 2>&1").c_str(), "r");
    if (pipe == NULL) {
        return result;
    }
    char buffer[4096];
    size_t count;
    while ((count = fread(buffer, 1, sizeof(buffer), pipe)) > 0) {
        result.output.append(buffer, count);
    }
    int status = pclose(pipe);
    result.exitCode = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    return result;
}

std::string quoted(const std::string& path) {
    return "'" + path + "'";
}

void writeFile(const std::string& path, const std::string& text) {
    std::ofstream(path.c_str()) << text;
}

// "002: 2f0  decsz 10" -> "org 2\ndecsz 10\n" for every line of a listing
std::string listingToSource(const std::string& listing) {
    std::istringstream in(listing);
    std::ostringstream source;
    std::string line;
    while (std::getline(in, line)) {
        size_t colon = line.find(':');
        size_t text = line.find("  ", colon);
        std::string instruction = line.substr(text + 2);
        if (instruction.compare(0, 8, "not impl") == 0) {
            continue;       // data, there is no source for it
        }
        source << "org " << line.substr(0, colon) << "\n" << instruction << "\n";
    }
    return source.str();
}

}

class ToolsTest : public ::testing::Test {
protected:
    std::string dir = testing::TempDir();

    std::string sxasm(const std::string& arguments) {
        return quoted(SXASM_PATH) + " " + arguments;
    }
    std::string sxdis(const std::string& arguments) {
        return quoted(SXDIS_PATH) + " " + arguments;
    }
};

TEST_F(ToolsTest, AssembledProgramIsListedAsWritten) {
    writeFile(dir + "/small.asm",
        "counter equ 10\n"
        "start:  mov w, #5\n"
        "        mov counter, w\n"
        "loop:   decsz counter\n"
        "        jmp loop\n"
        "        jmp start\n");
    Result assembled = run(sxasm(quoted(dir + "/small.asm") + " " + quoted(dir + "/small.hex")));
    ASSERT_EQ(0, assembled.exitCode) << assembled.output;

    Result listing = run(sxdis("-n " + quoted(dir + "/small.hex")));
    EXPECT_EQ(0, listing.exitCode);
    EXPECT_EQ(
        "000: c05  mov w, #5\n"
        "001: 030  mov 10, w\n"
        "002: 2f0  decsz 10\n"
        "003: a02  jmp 2\n"
        "004: a00  jmp 0\n",
        listing.output);
}

TEST_F(ToolsTest, SyntaxErrorShowsTheLineAndFails) {
    writeFile(dir + "/bad.asm", "nop\njmp nowhere\n");
    Result result = run(sxasm(quoted(dir + "/bad.asm") + " " + quoted(dir + "/bad.hex")));
    EXPECT_EQ(1, result.exitCode);
    EXPECT_NE(std::string::npos, result.output.find("line 2: unknown name nowhere")) << result.output;
}

TEST_F(ToolsTest, MissingSourceFileFails) {
    Result result = run(sxasm(quoted(dir + "/missing.asm") + " " + quoted(dir + "/out.hex")));
    EXPECT_EQ(1, result.exitCode);
}

TEST_F(ToolsTest, MissingArgumentsShowTheUsage) {
    EXPECT_EQ(2, run(sxasm("")).exitCode);
    EXPECT_EQ(2, run(sxdis("")).exitCode);
}

TEST_F(ToolsTest, MissingHexFileFails) {
    EXPECT_EQ(1, run(sxdis(quoted(dir + "/missing.hex"))).exitCode);
}

TEST_F(ToolsTest, TargetsOfJumpsAreShownAsLabels) {
    writeFile(dir + "/small.asm",
        "start:  mov w, #5\n"
        "        mov 10, w\n"
        "loop:   decsz 10\n"
        "        jmp loop\n"
        "        jmp start\n");
    ASSERT_EQ(0, run(sxasm(quoted(dir + "/small.asm") + " " + quoted(dir + "/small.hex"))).exitCode);

    Result listing = run(sxdis(quoted(dir + "/small.hex")));
    EXPECT_EQ(0, listing.exitCode);
    EXPECT_EQ(
        "L000:\n"
        "000: c05  mov w, #5\n"
        "001: 030  mov 10, w\n"
        "L002:\n"
        "002: 2f0  decsz 10\n"
        "003: a02  jmp L002\n"
        "004: a00  jmp L000\n",
        listing.output);
}

TEST_F(ToolsTest, LabelsFollowThePageInstruction) {
    writeFile(dir + "/far.asm", "autopage on\ncall sub\njmp far\norg 200\nsub: ret\nfar: nop\n");
    ASSERT_EQ(0, run(sxasm(quoted(dir + "/far.asm") + " " + quoted(dir + "/far.hex"))).exitCode);

    Result listing = run(sxdis(quoted(dir + "/far.hex")));
    EXPECT_EQ(
        "000: 011  page 1\n"
        "001: 900  call L200\n"
        "002: 011  page 1\n"
        "003: a01  jmp L201\n"
        "L200:\n"
        "200: 00c  ret\n"
        "L201:\n"
        "201: 000  nop\n",
        listing.output);
}

TEST_F(ToolsTest, TargetWithoutCodeStaysANumber) {
    writeFile(dir + "/empty.asm", "jmp 5\n");
    ASSERT_EQ(0, run(sxasm(quoted(dir + "/empty.asm") + " " + quoted(dir + "/empty.hex"))).exitCode);

    Result listing = run(sxdis(quoted(dir + "/empty.hex")));
    EXPECT_EQ("000: a05  jmp 5\n", listing.output);
}

TEST_F(ToolsTest, UnknownOptionShowsTheUsage) {
    EXPECT_EQ(2, run(sxdis("-x " + quoted(dir + "/small.hex"))).exitCode);
}

// A real program survives listing, assembling and listing again.
class ToolsExampleTest : public ToolsTest, public ::testing::WithParamInterface<const char*> {};

TEST_P(ToolsExampleTest, ListingOfTheAssembledListingIsTheSame) {
    std::string original = std::string(EXAMPLES_DIR) + "/" + GetParam();
    Result first = run(sxdis("-n " + quoted(original)));
    ASSERT_EQ(0, first.exitCode);

    writeFile(dir + "/round.asm", listingToSource(first.output));
    Result assembled = run(sxasm(quoted(dir + "/round.asm") + " " + quoted(dir + "/round.hex")));
    ASSERT_EQ(0, assembled.exitCode) << assembled.output;

    Result second = run(sxdis("-n " + quoted(dir + "/round.hex")));
    ASSERT_EQ(0, second.exitCode);

    // the words without source are left out of the second listing
    std::istringstream in(first.output);
    std::string line, expected;
    while (std::getline(in, line)) {
        if (line.find("not impl") == std::string::npos) {
            expected += line + "\n";
        }
    }
    EXPECT_EQ(expected, second.output);
}

INSTANTIATE_TEST_SUITE_P(Examples, ToolsExampleTest,
    ::testing::Values("kj_pong_01.hex", "ms_pac_man.hex", "rem_tetris_01.hex", "racer_01.hex"));

#endif
