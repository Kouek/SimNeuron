#include <QtWidgets/qapplication.h>

#include "main_window.hpp"

using namespace kouek;

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    MainWindow window;
    window.showMaximized();
    return app.exec();
}
