#pragma once

#include <string>
#include <vector>

struct GpgProcessResult {
    int exitCode = -1;
    std::string standardOutput;
    std::string standardError;
    std::string errorMessage;

    bool success() const { return exitCode == 0 && errorMessage.empty(); }
};

class GpgProcess {
public:
    static GpgProcessResult run(const std::string& executable,
                                const std::vector<std::string>& arguments,
                                const std::string& standardInput = {});
};
