#pragma once

#include "ModType.h"
#include "http_client.h"
#include "pathing.h"

#include <QObject>
#include <QList>
#include <QDir>
#include <QJsonArray>
#include <QJsonObject>
#include <memory>

class Manager : public QObject {
    Q_OBJECT

public:
    explicit Manager(QObject* parent = nullptr);

    // Installed mods
    void scanInstalledMods();
    bool uninstallMod(const QString& id);
    QList<ModInfo> getInstalledMods() const;
    ModInfo* getInstalledMod(const QString& id);

    // Available mods
    void loadAvailableMods();
    QList<ModInfo> getAvailableMods() const;
    QList<ModInfo> getModsWithUpdates() const;
    ModInfo* getAvailableMod(const QString& id);

    bool installMod(const QString& id);
    bool updateMod(const QString& id);

signals:
    void installedModsChanged();
    void availableModsChanged();
    void modActionStarted(const QString& action, const QString& modTitle);
    void modActionCompleted(const QString& action, const QString& modTitle, bool success);

private:
    Pathing* m_pathing;
    QDir m_addonsDir;

    QList<ModInfo> mods;
    HttpClient* httpClient;
    ModInfo parseInstalledMod(const QDir& dir);
    ModInfo parseAvailableMod(const QJsonObject& obj);
    void parseAvailableMods(const QString& filePath);
    //void updateModComparisons();
};