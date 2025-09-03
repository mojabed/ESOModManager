#include "esomm.h"
#include "logger.h"
#include "pathing.h"
#include "esomm_style.h"

#include <QtWidgets/QApplication>
#include <QIcon>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    Logger::init();
    Pathing::getPaths(); // gets systems paths and ESO AddOns directory path

    //StyleLoader::apply(); // global config: esomm_style.qss

    ESOMM mainWindow;
    mainWindow.setWindowIcon(QIcon(":/esomm.ico"));
    mainWindow.show();

    qCInfo(loggerCategory) << "ESOMM window opened successfully" << Qt::endl;

    return app.exec();
}
