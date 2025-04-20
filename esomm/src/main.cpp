#include "esomm.h"
#include "logger.h"
#include <QtWidgets/QApplication>
#include <QIcon>
#include <QLoggingCategory>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    Logger::init();

    ESOMM mainWindow;
    mainWindow.setWindowIcon(QIcon(":/esomm.ico"));
    mainWindow.show();

    qCInfo(loggerCategory) << "ESOMM window opened successfully" << Qt::endl;

    return app.exec();
}
