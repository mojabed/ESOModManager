#include "pathing.h"

#include <QStandardPaths>
#include <QFileInfo>
#include <QDir>

Pathing* Pathing::paths = nullptr;
QMutex Pathing::mutex;

Pathing::Pathing() {
    docsPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    addonsPath = docsPath + "/Elder Scrolls Online/live/AddOns";
    appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    appConfigPath = appDataPath + "/config";

    fprintf(stderr, "App data path: %s\n", appDataPath.toUtf8().constData());
    fprintf(stderr, "Config path: %s\n", appConfigPath.toUtf8().constData());
    fflush(stderr);

    try {
        QDir appDataDir(appDataPath);
        if (!appDataDir.exists()) {
            fprintf(stderr, "App data directory does not exist, creating at %s\n", appDataPath.toUtf8().constData());
            if (!appDataDir.mkpath(appDataPath)) {
                fprintf(stderr, "Failed to create app data directory at %s\n", appDataPath.toUtf8().constData());
            }
        }

        QDir configDir(appConfigPath);
        if (!configDir.exists()) {
            fprintf(stderr, "Config directory does not exist, creating at %s\n", appConfigPath.toUtf8().constData());
            if (!configDir.mkpath(appConfigPath)) {
                fprintf(stderr, "Failed to create config directory at %s\n", appConfigPath.toUtf8().constData());
            }
        }

        QDir dir(addonsPath);
        if (!dir.exists()) {
            fprintf(stderr, "AddOns directory does not exist, creating at %s\n", addonsPath.toUtf8().constData());
            if (!dir.mkpath(addonsPath)) {
                fprintf(stderr, "Failed to create AddOns directory at %s\n", addonsPath.toUtf8().constData());
            }
        }
        fprintf(stderr, "AddOns directory located at %s\n", addonsPath.toUtf8().constData());
        fflush(stderr);
        // Need to confirm with user if this is the correct path, get input from file dialog if not

    } catch (const std::exception& e) {
        fprintf(stderr, "Exception while creating AddOns directory: %s\n", e.what());
        fflush(stderr);
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

QString Pathing::getAppConfigPath() {
    return appConfigPath;
}

QString Pathing::getAppDataPath() {
    return appDataPath;
}