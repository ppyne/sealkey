#pragma once

#include <string>
#include <vector>

enum class GpgOutputStream {
    Stdout,
    Stderr
};

struct GpgOutputChunk {
    GpgOutputStream stream = GpgOutputStream::Stdout;
    std::string text;
};

struct GpgProcessResult {
    int exitCode = -1;
    std::string standardOutput;
    std::string standardError;
    std::vector<GpgOutputChunk> outputChunks;
    std::string errorMessage;

    bool success() const { return exitCode == 0 && errorMessage.empty(); }
};

class GpgProcess {
public:
    static GpgProcessResult run(const std::string& executable,
                                const std::vector<std::string>& arguments,
                                const std::string& standardInput = {});
};
