#include "MainWindow.hpp"

#include <FL/Fl.H>

int main(int argc, char** argv) {
    auto window = new MainWindow();
    window->show(argc, argv);
    return Fl::run();
}
