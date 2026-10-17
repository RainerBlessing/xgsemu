#ifndef TEST_FILE_H
#define TEST_FILE_H

#include <stdio.h>
#include <string>
#include <vector>

// A file in the working directory that is removed again when the object goes
// away. The name contains the name of the running test, so tests that run one
// after the other do not meet the leftovers of each other.
class TestFile {
public:
    TestFile(const std::string& suffix, const std::string& contents = "") {
        const ::testing::TestInfo* info = ::testing::UnitTest::GetInstance()->current_test_info();
        path = std::string("tmp_") + info->test_suite_name() + "_" + info->name() + suffix;
        write(contents);
    }

    ~TestFile() {
        remove(path.c_str());
    }

    void write(const std::string& contents) {
        FILE* f = fopen(path.c_str(), "wb");
        if (f) {
            fwrite(contents.data(), 1, contents.size(), f);
            fclose(f);
        }
    }

    std::string read() const {
        std::string contents;
        FILE* f = fopen(path.c_str(), "rb");
        if (!f) {
            return contents;
        }
        char buffer[4096];
        size_t n;
        while ((n = fread(buffer, 1, sizeof(buffer), f)) > 0) {
            contents.append(buffer, n);
        }
        fclose(f);
        return contents;
    }

    const char* name() const {
        return path.c_str();
    }

private:
    std::string path;
};

// One Intel hex record with the correct checksum.
inline std::string hexRecord(unsigned address, unsigned type, const std::vector<int>& data) {
    char buffer[16];
    std::string line = ":";
    int sum = 0;
    int header[4] = {(int)data.size(), (int)(address >> 8) & 0xff, (int)address & 0xff, (int)type};
    for (int i = 0; i < 4; i++) {
        snprintf(buffer, sizeof(buffer), "%02X", header[i]);
        line += buffer;
        sum += header[i];
    }
    for (size_t i = 0; i < data.size(); i++) {
        snprintf(buffer, sizeof(buffer), "%02X", data[i] & 0xff);
        line += buffer;
        sum += data[i];
    }
    snprintf(buffer, sizeof(buffer), "%02X", (-sum) & 0xff);
    return line + buffer + "\n";
}

#endif
