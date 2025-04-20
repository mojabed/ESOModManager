#include "pathing.h"
#include "logger.h"

#include <QStandardPaths>
#include <QFileInfo>
#include <QDir>

Pathing* Pathing::paths = nullptr;
QMutex Pathing::mutex;

Pathing::Pathing() {
    docsPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    addonsPath = docsPath.append("/Elder Scrolls Online/live/AddOns");
    // Create addons directory if it doesn't exist
    try {
        QDir dir(addonsPath);
        if (!dir.exists()) {
            qCInfo(loggerCategory) << "AddOns directory does not exist, creating at " << addonsPath;
            if (!dir.mkpath(addonsPath)) {
                qCWarning(loggerCategory) << "Failed to create AddOns directory at " << addonsPath;
            }
        }
        qCInfo(loggerCategory) << "AddOns directory located at " << addonsPath;
        // Need to confirm with user if this is the correct path, get input from file dialog if not

    } catch (const std::exception& e) {
        qCWarning(loggerCategory) << "Exception while creating AddOns directory: " << e.what();
    }
}

bool Pathing::doesExist(const QString& path) {
    QDir dirInfo(path);
    return dirInfo.exists();
}

bool Pathing::isWritable(const QString& path) {
    return QFileInfo(path).isWritable();
}

QString Pathing::getUserDataPath() {
    return docsPath;
}

QString Pathing::getAddonsPath() {
    return addonsPath;
}