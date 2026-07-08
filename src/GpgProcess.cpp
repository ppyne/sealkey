#include "GpgProcess.hpp"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <functional>
#include <mutex>
#include <sstream>
#include <thread>

namespace {
std::string quoteWindowsArgument(const std::string& arg) {
    if (arg.empty()) {
        return "\"\"";
    }
    bool needsQuotes = arg.find_first_of(" \t\n\v\"") != std::string::npos;
    if (!needsQuotes) {
        return arg;
    }
    std::string result = "\"";
    unsigned backslashes = 0;
    for (char c : arg) {
        if (c == '\\') {
            ++backslashes;
        } else if (c == '"') {
            result.append(backslashes * 2 + 1, '\\');
            result.push_back('"');
            backslashes = 0;
        } else {
            result.append(backslashes, '\\');
            backslashes = 0;
            result.push_back(c);
        }
    }
    result.append(backslashes * 2, '\\');
    result.push_back('"');
    return result;
}

void readHandle(HANDLE handle, GpgOutputStream stream, GpgProcessResult& result, std::mutex& mutex) {
    char buffer[4096];
    DWORD read = 0;
    while (ReadFile(handle, buffer, sizeof(buffer), &read, nullptr) && read > 0) {
        std::string text(buffer, buffer + read);
        std::lock_guard<std::mutex> lock(mutex);
        if (stream == GpgOutputStream::Stdout) {
            result.standardOutput += text;
        } else {
            result.standardError += text;
        }
        result.outputChunks.push_back({stream, std::move(text)});
    }
}
}

GpgProcessResult GpgProcess::run(const std::string& executable,
                                 const std::vector<std::string>& arguments,
                                 const std::string& standardInput) {
    GpgProcessResult result;
    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    HANDLE outRead = nullptr;
    HANDLE outWrite = nullptr;
    HANDLE errRead = nullptr;
    HANDLE errWrite = nullptr;
    HANDLE inRead = nullptr;
    HANDLE inWrite = nullptr;
    if (!CreatePipe(&outRead, &outWrite, &sa, 0) ||
        !CreatePipe(&errRead, &errWrite, &sa, 0) ||
        !CreatePipe(&inRead, &inWrite, &sa, 0)) {
        result.errorMessage = "Unable to create process pipes.";
        return result;
    }
    SetHandleInformation(outRead, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(errRead, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(inWrite, HANDLE_FLAG_INHERIT, 0);

    std::ostringstream commandLine;
    commandLine << quoteWindowsArgument(executable);
    for (const auto& arg : arguments) {
        commandLine << ' ' << quoteWindowsArgument(arg);
    }
    auto mutableCommandLine = commandLine.str();

    STARTUPINFOA startup{};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdOutput = outWrite;
    startup.hStdError = errWrite;
    startup.hStdInput = inRead;

    PROCESS_INFORMATION process{};
    BOOL created = CreateProcessA(nullptr,
                                  mutableCommandLine.data(),
                                  nullptr,
                                  nullptr,
                                  TRUE,
                                  0,
                                  nullptr,
                                  nullptr,
                                  &startup,
                                  &process);
    CloseHandle(outWrite);
    CloseHandle(errWrite);
    CloseHandle(inRead);

    if (!created) {
        CloseHandle(outRead);
        CloseHandle(errRead);
        CloseHandle(inWrite);
        result.errorMessage = "Unable to start GPG executable.";
        return result;
    }

    if (!standardInput.empty()) {
        DWORD written = 0;
        WriteFile(inWrite, standardInput.data(), static_cast<DWORD>(standardInput.size()), &written, nullptr);
    }
    CloseHandle(inWrite);

    std::mutex outputMutex;
    std::thread stdoutThread(readHandle, outRead, GpgOutputStream::Stdout, std::ref(result), std::ref(outputMutex));
    std::thread stderrThread(readHandle, errRead, GpgOutputStream::Stderr, std::ref(result), std::ref(outputMutex));
    stdoutThread.join();
    stderrThread.join();
    CloseHandle(outRead);
    CloseHandle(errRead);

    WaitForSingleObject(process.hProcess, INFINITE);
    DWORD code = 1;
    GetExitCodeProcess(process.hProcess, &code);
    result.exitCode = static_cast<int>(code);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return result;
}
#else
#include <sys/select.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <vector>

namespace {
void closeIfValid(int fd) {
    if (fd >= 0) {
        close(fd);
    }
}
}

GpgProcessResult GpgProcess::run(const std::string& executable,
                                 const std::vector<std::string>& arguments,
                                 const std::string& standardInput) {
    GpgProcessResult result;
    int stdinPipe[2]{-1, -1};
    int stdoutPipe[2]{-1, -1};
    int stderrPipe[2]{-1, -1};
    if (pipe(stdinPipe) != 0 || pipe(stdoutPipe) != 0 || pipe(stderrPipe) != 0) {
        result.errorMessage = std::string("Unable to create process pipes: ") + strerror(errno);
        closeIfValid(stdinPipe[0]);
        closeIfValid(stdinPipe[1]);
        closeIfValid(stdoutPipe[0]);
        closeIfValid(stdoutPipe[1]);
        closeIfValid(stderrPipe[0]);
        closeIfValid(stderrPipe[1]);
        return result;
    }

    pid_t pid = fork();
    if (pid < 0) {
        result.errorMessage = std::string("Unable to fork: ") + strerror(errno);
        closeIfValid(stdinPipe[0]);
        closeIfValid(stdinPipe[1]);
        closeIfValid(stdoutPipe[0]);
        closeIfValid(stdoutPipe[1]);
        closeIfValid(stderrPipe[0]);
        closeIfValid(stderrPipe[1]);
        return result;
    }

    if (pid == 0) {
        dup2(stdinPipe[0], STDIN_FILENO);
        dup2(stdoutPipe[1], STDOUT_FILENO);
        dup2(stderrPipe[1], STDERR_FILENO);
        closeIfValid(stdinPipe[0]);
        closeIfValid(stdinPipe[1]);
        closeIfValid(stdoutPipe[0]);
        closeIfValid(stdoutPipe[1]);
        closeIfValid(stderrPipe[0]);
        closeIfValid(stderrPipe[1]);

        std::vector<char*> argv;
        argv.reserve(arguments.size() + 2);
        argv.push_back(const_cast<char*>(executable.c_str()));
        for (const auto& argument : arguments) {
            argv.push_back(const_cast<char*>(argument.c_str()));
        }
        argv.push_back(nullptr);
        execvp(executable.c_str(), argv.data());
        _exit(127);
    }

    closeIfValid(stdinPipe[0]);
    closeIfValid(stdoutPipe[1]);
    closeIfValid(stderrPipe[1]);

    if (!standardInput.empty()) {
        const char* data = standardInput.data();
        std::size_t remaining = standardInput.size();
        while (remaining > 0) {
            ssize_t written = write(stdinPipe[1], data, remaining);
            if (written <= 0) {
                break;
            }
            data += written;
            remaining -= static_cast<std::size_t>(written);
        }
    }
    closeIfValid(stdinPipe[1]);

    bool stdoutOpen = true;
    bool stderrOpen = true;
    while (stdoutOpen || stderrOpen) {
        fd_set readSet;
        FD_ZERO(&readSet);
        int maxFd = -1;
        if (stdoutOpen) {
            FD_SET(stdoutPipe[0], &readSet);
            maxFd = std::max(maxFd, stdoutPipe[0]);
        }
        if (stderrOpen) {
            FD_SET(stderrPipe[0], &readSet);
            maxFd = std::max(maxFd, stderrPipe[0]);
        }
        if (select(maxFd + 1, &readSet, nullptr, nullptr, nullptr) < 0) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }
        if (stdoutOpen && FD_ISSET(stdoutPipe[0], &readSet)) {
            char buffer[4096];
            ssize_t count = read(stdoutPipe[0], buffer, sizeof(buffer));
            if (count > 0) {
                result.standardOutput.append(buffer, buffer + count);
                result.outputChunks.push_back({GpgOutputStream::Stdout, std::string(buffer, buffer + count)});
            } else {
                stdoutOpen = false;
                closeIfValid(stdoutPipe[0]);
            }
        }
        if (stderrOpen && FD_ISSET(stderrPipe[0], &readSet)) {
            char buffer[4096];
            ssize_t count = read(stderrPipe[0], buffer, sizeof(buffer));
            if (count > 0) {
                result.standardError.append(buffer, buffer + count);
                result.outputChunks.push_back({GpgOutputStream::Stderr, std::string(buffer, buffer + count)});
            } else {
                stderrOpen = false;
                closeIfValid(stderrPipe[0]);
            }
        }
    }

    int status = 0;
    while (waitpid(pid, &status, 0) < 0 && errno == EINTR) {
    }
    if (WIFEXITED(status)) {
        result.exitCode = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        result.exitCode = 128 + WTERMSIG(status);
    }
    return result;
}
#endif
