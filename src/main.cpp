#include "AppInfo.hpp"
#include "MainWindow.hpp"

#include <FL/Fl.H>
#include <FL/Fl_Window.H>

#if defined(__linux__)
#include <FL/Fl_PNG_Image.H>

#include <filesystem>
#include <memory>
#include <vector>
#endif

namespace {
#if defined(__linux__)
std::filesystem::path installedIconPathFromExecutable(const char* executable) {
    if (!executable || !*executable) {
        return {};
    }

    std::error_code ec;
    auto path = std::filesystem::weakly_canonical(executable, ec);
    if (ec || path.empty() || !path.is_absolute()) {
        return {};
    }

    return path.parent_path().parent_path() / "share" / "icons" / "hicolor" / "256x256" / "apps" / "sealkey.png";
}

void configureLinuxWindowIcon(const char* executable) {
    static std::unique_ptr<Fl_PNG_Image> icon;
    std::vector<std::filesystem::path> candidates{
        installedIconPathFromExecutable(executable),
        SEALKEY_INSTALLED_ICON_PATH,
        SEALKEY_SOURCE_ICON_PATH,
    };

    for (const auto& path : candidates) {
        std::error_code ec;
        if (path.empty() || !std::filesystem::is_regular_file(path, ec)) {
            continue;
        }

        auto candidate = std::make_unique<Fl_PNG_Image>(path.string().c_str());
        if (candidate->w() > 0 && candidate->h() > 0) {
            icon = std::move(candidate);
            Fl_Window::default_icon(icon.get());
            return;
        }
    }
}
#else
void configureLinuxWindowIcon(const char*) {}
#endif
}

int main(int argc, char** argv) {
    Fl_Window::default_xclass(AppInfo::ProjectId);
    configureLinuxWindowIcon(argc > 0 ? argv[0] : nullptr);
    auto window = new MainWindow();
    window->show(argc, argv);
    return Fl::run();
}
