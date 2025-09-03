#pragma once

#include <QString>
#include <QDateTime>
#include <QUrl>
#include <QList>

struct Dependancies {
    QString path;
    QString addOnVersion;
    QString apiVersion;
    bool library = false;
    QList<QString> optionalDependencies;
    QList<QString> requiredDependencies;

};

struct ModInfo {
    // Common properties
    QString id;
    QString categoryId;
    QString version;
    QString lastUpdate;
    QString title;
    QString author;
    QString fileInfoUri;
    QList<QString> gameVersions;
    QString checksum;
    QList<Dependancies> addons;
    bool library = false;
    QUrl donationUrl;

    QDateTime lastUpdated;

    // Installed mod properties
    bool isInstalled = false;
    QString installPath;
    qint64 sizeInBytes = 0;

    // Available mod properties
    QUrl downloadUrl;
    int downloads = 0;
    int downloadsMonthly = 0;
    int favorites = 0;
    bool hasUpdate = false;

    QString getFormattedDate() const {
        return lastUpdated.isValid() ? lastUpdated.toString("dd-MM-yyyy") : "";
    }
};