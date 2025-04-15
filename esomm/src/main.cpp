#include "esomm.h"
#include <QtWidgets/QApplication>
#include <QIcon>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    ESOMM mainWindow;
    mainWindow.setWindowIcon(QIcon(":/esomm.ico"));
    mainWindow.show();

    return app.exec();
}
