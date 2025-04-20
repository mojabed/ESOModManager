#pragma once

#include <QMutex>
#include <QMutexLocker>
#include <QString>
#include <QStringList>
#include <QDir>

class Pathing {
public:
    // Enforce singleton
    Pathing(const Pathing&) = delete;
    Pathing& operator=(const Pathing&) = delete;

    static Pathing* getPaths() {
        if (paths == nullptr) {
            QMutexLocker locker(&mutex);
            if (paths == nullptr) {
                paths = new Pathing();
            }
        }
        return paths;
    }

    bool doesExist(const QString& path);
    bool isWritable(const QString& path);
    QString getUserDataPath();
    QString getAddonsPath();

private:
    QString docsPath, addonsPath;
    static Pathing* paths;
    static QMutex mutex;

    Pathing();
};