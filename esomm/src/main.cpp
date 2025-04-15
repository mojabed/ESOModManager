#include "esomm.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    ESOMM mainWindow;
    mainWindow.show();

    return app.exec();
}
