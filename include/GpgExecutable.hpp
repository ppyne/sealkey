#pragma once

#include <string>
#include <vector>

struct GpgTestResult {
    bool valid = false;
    int exitCode = -1;
    std::string versionText;
    std::string errorText;
};

class GpgExecutable {
public:
    explicit GpgExecutable(std::string path = {});

    const std::string& path() const;
    void setPath(std::string path);
    bool empty() const;

    GpgTestResult test() const;
    static std::string detect();
    static std::vector<std::string> candidatePaths();

private:
    std::string path_;
};
