#include "manager.h"
#include "logger.h"
#include "pathing.h"

#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QFileInfo>   

Manager::Manager(QObject* parent)
    : QObject(parent), httpClient(new HttpClient(4, this)) {

    m_pathing = Pathing::getPaths();
    m_addonsDir = QDir(m_pathing->getAddonsPath());

    loadInstalledModsCache();

    connect(httpClient, &HttpClient::downloadFinished,
        [this](const QString& filePath) {
            qCInfo(loggerCategory) << "Download completed:" << filePath;

            QString masterJsonPath = m_pathing->getAppDataPath() + "/master.json";
            if (filePath == masterJsonPath) {
                parseAvailableMods(masterJsonPath);
            } else {
                // Extract mod ID from the file path
                QFileInfo fileInfo(filePath);
                QString modId = fileInfo.baseName();

                emit modActionCompleted("install", modId, true);
            }
        });

    connect(httpClient, &HttpClient::downloadFailed,
        [this](const QString& filePath, const QString& error) {
            qCWarning(loggerCategory) << "Download failed:" << filePath << "-" << error;

            QString masterJsonPath = m_pathing->getAppDataPath() + "/master.json";
            if (filePath == masterJsonPath) {
                QFile existingFile(masterJsonPath);
                if (existingFile.exists()) {
                    parseAvailableMods(masterJsonPath);
                } else {
                    qCWarning(loggerCategory) << "No existing master mod list available";
                    emit availableModsChanged();
                }
            } else { // Mod download failed
                QFileInfo fileInfo(filePath);
                QString modId = fileInfo.baseName();

                emit modActionCompleted("install", modId, false);
            }
        });
}

bool operator==(const ModInfo &a, const QString &b) {
    return a.title == b;
}
bool operator==(const ModInfo& a, const ModInfo& b) {
    return a.title == b.title;
}

void Manager::scanInstalledMods() {
    qCInfo(loggerCategory) << "Scanning installed mods in: " << m_addonsDir.absolutePath();

    if (!installedMods.isEmpty()) {
        installedMods.clear();
    }
    if (!m_addonsDir.exists()) {
        qCWarning(loggerCategory) << "AddOns directory does not exist: " << m_addonsDir.absolutePath();
        emit installedModsChanged();
        return;
    }

    QStringList subDirs = m_addonsDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    int numInstalled = 0;

    for (const QString& subDir : subDirs) {
        QDir modDir(m_addonsDir.absoluteFilePath(subDir));
        const QString modName = modDir.dirName();
        const QString modPath = modDir.absolutePath();

        for (int i = 0; i < mods.size(); i++) {
            if (mods[i].title == modName) {
                mods[i].isInstalled = true;
                mods[i].installPath = modPath;
                qCInfo(loggerCategory) << "Found installed mod:" << mods[i].title << "at" << mods[i].installPath;
                installedMods[modName] = &mods[i];

                numInstalled++;
                break;
            }

        }
        
    }

    qCInfo(loggerCategory) << "Scanned " << numInstalled << " installed mods.";

    //updateModComparisons();
    emit installedModsChanged();
}

ModInfo Manager::parseInstalledMod(const QDir& directory) {
    ModInfo mod;
    mod.isInstalled = true;
    mod.installPath = directory.absolutePath();

    return mod;
}

QList<ModInfo> Manager::getInstalledMods() const {
    QList<ModInfo> result;
    for (ModInfo* mod : installedMods.values()) {
        if (!result.contains(*mod)) {
            qCInfo(loggerCategory) << "getInstalledMods found installed mod:" << mod->title;
            result.append(*mod);
        }
    }
    qCInfo(loggerCategory) << "getInstalledMods found " << result.size() << " installed mods";
    return result;
}

ModInfo* Manager::getInstalledMod(const QString& id) {
    if (installedMods.contains(id)) {
        return installedMods[id];
    }

    return nullptr;
}

bool Manager::uninstallMod(const QString& id) {
    ModInfo* mod = getInstalledMod(id);
    if (!mod) {
        return false;
    }

    emit modActionStarted("uninstall", mod->title);

    QDir dir(mod->installPath);

    bool success = false;
    if (dir.exists()) {
        success = dir.removeRecursively();
        if (success) {
            qCInfo(loggerCategory) << "Uninstalled mod:" << mod->title;

            mods.removeIf([id](const ModInfo& m) {
                return m.isInstalled && (m.id == id || (id.isEmpty() && m.title == id));
            });

            emit installedModsChanged();
        } else {
            qCWarning(loggerCategory) << "Failed to uninstall mod:" << mod->title;
        }
    }

    emit modActionCompleted("uninstall", mod->title, success);
    return success;
}


// Reads the manifest json file (master.json) for all ESOUI addons
void Manager::parseAvailableMods(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qCWarning(loggerCategory) << "Failed to open master.json file:" << file.errorString();
        //emit availableModsChanged();
        return;
    }
    QByteArray jsonData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(jsonData, &parseError);

    // Emit empty list if parsing fails
    if (parseError.error != QJsonParseError::NoError) {
        qCWarning(loggerCategory) << "Failed to parse master.json:" << parseError.errorString();
        emit availableModsChanged();
        return;
    }

    if (!document.isArray()) {
        qCWarning(loggerCategory) << "master.json is not a valid JSON array";
        emit availableModsChanged();
        return;
    }

    QJsonArray modsJsonArray = document.array();

    for (const QJsonValue& value : modsJsonArray) {
        qCInfo(loggerCategory) << "Processing mod entry:" << value.toString();
        QJsonObject modObj = value.toObject();

        ModInfo mod = parseAvailableMod(modObj);
        mods.append(mod);
    }

    qCInfo(loggerCategory) << "Loaded" << getAvailableMods().size() << "available mods";
    //updateModComparisons();
    emit availableModsChanged();
    emit availableModsLoaded();
}

void Manager::loadAvailableMods() {
    qCInfo(loggerCategory) << "Loading available mods";

    QUrl masterUrl("https://api.mmoui.com/v4/game/ESO/filelist.json");
    QString masterJsonPath = m_pathing->getAppDataPath() + "/master.json";

    httpClient->addDownload(masterUrl, masterJsonPath);
}

ModInfo Manager::parseAvailableMod(const QJsonObject& jsonData) {
    ModInfo mod;
    mod.isInstalled = false;

    // Basic mod info
    mod.id = jsonData["id"].toString();
    mod.categoryId = jsonData["categoryId"].toString();
    mod.version = jsonData["version"].toString();
    mod.lastUpdate = jsonData["lastUpdate"].toString();
    mod.title = jsonData["title"].toString();
    mod.author = jsonData["author"].toString();
    mod.fileInfoUri = jsonData["fileInfoUri"].toString();
    mod.downloads = jsonData["downloads"].toInt();
    mod.downloadsMonthly = jsonData["downloadsMonthly"].toInt();
    mod.checksum = jsonData["checksum"].toString();
    mod.library = jsonData["library"].toBool(false);
    mod.donationUrl = jsonData.contains("donationUri") ? QUrl(jsonData["donationUri"].toString()) : QUrl();
    mod.downloadUrl = QUrl(jsonData["downloadUri"].toString());

    if (jsonData.contains("gameVersions") && jsonData["gameVersions"].isArray()) {
        QJsonArray versionsArray = jsonData["gameVersions"].toArray();
        QStringList versions;
        for (const QJsonValue& version : versionsArray) {
            versions.append(version.toString());
        }
        mod.gameVersions = versions;
    } else {
        mod.gameVersions = QStringList();
    }

    if (jsonData.contains("addons") && jsonData["addons"].isArray()) {
        QJsonArray addonsArray = jsonData["addons"].toArray();
        QList<Dependancies> addonsList;

        for (const QJsonValue& addonValue : addonsArray) {
            if (addonValue.isObject()) {
                QJsonObject addonObj = addonValue.toObject();
                Dependancies dependency;

                dependency.path = addonObj["path"].toString();
                dependency.addOnVersion = addonObj["addOnVersion"].toString();
                dependency.apiVersion = addonObj["apiVersion"].toString();
                dependency.library = addonObj.contains("library") ? addonObj["library"].toBool() : false;

                // Parse optional and required dependencies if present
                if (addonObj.contains("optionalDependencies") && addonObj["optionalDependencies"].isArray()) {
                    QJsonArray optDepArray = addonObj["optionalDependencies"].toArray();
                    for (const QJsonValue& depValue : optDepArray) {
                        dependency.optionalDependencies.append(depValue.toString());
                    }
                }

                if (addonObj.contains("requiredDependencies") && addonObj["requiredDependencies"].isArray()) {
                    QJsonArray reqDepArray = addonObj["requiredDependencies"].toArray();
                    for (const QJsonValue& depValue : reqDepArray) {
                        dependency.requiredDependencies.append(depValue.toString());
                    }
                }

                addonsList.append(dependency);
            }
        }

        mod.addons = addonsList;
    } else {
        mod.addons = QList<Dependancies>();
    }

    // Handle the date format
    QString lastUpdateStr = jsonData["lastUpdate"].toString();
    if (!lastUpdateStr.isEmpty()) {
        mod.lastUpdated = QDateTime::fromString(lastUpdateStr, Qt::ISODate);
        if (!mod.lastUpdated.isValid()) {
            mod.lastUpdated = QDateTime::fromString(lastUpdateStr, "yyyy-MM-dd");
        }
    }

    return mod;
}

QList<ModInfo> Manager::getAvailableMods() const {
    QList<ModInfo> result;
    for (const ModInfo& mod : mods) {
        if (!mod.isInstalled) {
            result.append(mod);
        }
    }
    return result;
}

QList<ModInfo> Manager::getModsWithUpdates() const {
    QList<ModInfo> result;
    for (const ModInfo& mod : mods) {
        if (mod.isInstalled && mod.hasUpdate) {
            result.append(mod);
        }
    }
    return result;
}

ModInfo* Manager::getAvailableMod(const QString& id) {
    for (auto& mod : mods) {
        if (!mod.isInstalled && mod.id == id) {
            return &mod;
        }
    }
    return nullptr;
}

bool Manager::installMod(const QString& id) {
    ModInfo* mod = getAvailableMod(id);
    if (!mod || mod->downloadUrl.isEmpty()) {
        qCWarning(loggerCategory) << "Attempted to install invalid mod:" << id;
        return false;
    }

    emit modActionStarted("install", mod->title);

    QString fileName = mod->title.isEmpty() ? id : mod->title;
    fileName = fileName.replace(" ", "_").replace("/", "_");
    QString downloadPath = m_pathing->getAppDataPath() + "/downloads/" + fileName + ".zip";

    QDir downloadsDir(m_pathing->getAppDataPath() + "/downloads");
    if (!downloadsDir.exists()) {
        downloadsDir.mkpath(".");
    }

    httpClient->addDownload(mod->downloadUrl, downloadPath);

    // The download and installation completion will be handled in the httpClient signal handlers
    return true;
}

bool Manager::updateMod(const QString& id) {
    ModInfo* mod = getInstalledMod(id);
    if (!mod || !mod->hasUpdate) {
        qCWarning(loggerCategory) << "Attempted to update invalid mod:" << id;
        return false;
    }

    emit modActionStarted("update", mod->title);

    ModInfo* availableMod = nullptr;
    for (auto& m : mods) {
        if (!m.isInstalled && m.id == mod->id) {
            availableMod = &m;
            break;
        }
    }

    if (!availableMod || availableMod->downloadUrl.isEmpty()) {
        qCWarning(loggerCategory) << "Could not find available version for update:" << id;
        emit modActionCompleted("update", mod->title, false);
        return false;
    }

    // First uninstall the current version
    if (!uninstallMod(id)) {
        qCWarning(loggerCategory) << "Failed to uninstall mod for update:" << id;
        emit modActionCompleted("update", mod->title, false);
        return false;
    }

    // Then install the new version
    return installMod(availableMod->id);
}

QString Manager::getInstalledCachePath() const {
    return m_pathing->getAppDataPath() + "/installed_cache.json";
}

void Manager::saveInstalledModsCache() {
    QDir cacheDir(m_pathing->getAppDataPath());
    if (!cacheDir.exists()) {
        cacheDir.mkpath(".");
    }

    QJsonArray installedArray;
    for (ModInfo* mod : installedMods.values()) {
        if (mod) {
            installedArray.append(modToJson(*mod));
        }
    }

    QJsonDocument installedDoc(installedArray);
    QFile installedFile(getInstalledCachePath());
    if (installedFile.open(QIODevice::WriteOnly)) {
        installedFile.write(installedDoc.toJson());
        installedFile.close();
        qCInfo(loggerCategory) << "Saved " << installedMods.size() << " installed mods to cache.";
    } else {
        qCWarning(loggerCategory) << "Failed to save installed mods cache:" << installedFile.errorString();
    }
}

void Manager::loadInstalledModsCache() {
    QFile installedFile(getInstalledCachePath());

    if (!installedFile.exists() || !installedFile.open(QIODevice::ReadOnly)) {
        qCInfo(loggerCategory) << "Installed mods cache not found or could not be opened. Will perform a full scan.";
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(installedFile.readAll());
    installedFile.close();

    if (!doc.isArray()) {
        qCWarning(loggerCategory) << "Installed mods cache is not a valid JSON array.";
        installedFile.remove();
        return;
    }

    QJsonArray installedArray = doc.array();
    installedMods.clear();
    installedMods.reserve(installedArray.size());

    for (const QJsonValue& value : installedArray) {
        ModInfo mod = jsonToMod(value.toObject());
        installedMods[mod.title] = &mod;
    }

    qCInfo(loggerCategory) << "Loaded" << installedMods.size() << "installed mods from cache.";
    emit installedModsChanged();
}

QJsonObject Manager::modToJson(const ModInfo& mod) {
    QJsonObject json;

    json["id"] = mod.id;
    json["categoryId"] = mod.categoryId;
    json["version"] = mod.version;
    json["lastUpdate"] = mod.lastUpdate;
    json["title"] = mod.title;
    json["author"] = mod.author;
    json["fileInfoUri"] = mod.fileInfoUri;
    json["downloads"] = mod.downloads;
    json["downloadsMonthly"] = mod.downloadsMonthly;
    json["checksum"] = mod.checksum;
    json["library"] = mod.library;
    json["donationUrl"] = mod.donationUrl.toString();
    json["downloadUrl"] = mod.downloadUrl.toString();
    json["gameVersions"] = QJsonArray::fromStringList(mod.gameVersions);

    QJsonArray addonsArray;
    for (const auto& dep : mod.addons) {
        QJsonObject depObj;
        depObj["path"] = dep.path;
        depObj["addOnVersion"] = dep.addOnVersion;
        depObj["apiVersion"] = dep.apiVersion;
        depObj["library"] = dep.library;
        depObj["optionalDependencies"] = QJsonArray::fromStringList(dep.optionalDependencies);
        depObj["requiredDependencies"] = QJsonArray::fromStringList(dep.requiredDependencies);
    }
    json["addons"] = addonsArray;

    json["isInstalled"] = mod.isInstalled;
    json["installPath"] = mod.installPath;

    return json;
}

ModInfo Manager::jsonToMod(const QJsonObject& json) {
    ModInfo mod;

    mod.id = json["id"].toString();
    mod.categoryId = json["categoryId"].toString();
    mod.version = json["version"].toString();
    mod.lastUpdate = json["lastUpdate"].toString();
    mod.title = json["title"].toString();
    mod.author = json["author"].toString();
    mod.fileInfoUri = json["fileInfoUri"].toString();
    mod.checksum = json["checksum"].toString();
    mod.library = json["library"].toBool();
    mod.donationUrl = QUrl(json["donationUrl"].toString());
    mod.downloadUrl = QUrl(json["downloadUrl"].toString());

    QString lastUpdateStr = json["lastUpdate"].toString();
    if (!lastUpdateStr.isEmpty()) {
        mod.lastUpdated = QDateTime::fromString(lastUpdateStr, Qt::ISODate);
        if (!mod.lastUpdated.isValid()) {
            mod.lastUpdated = QDateTime::fromString(lastUpdateStr, "yyyy-MM-dd");
        }
    }

    if (json.contains("gameVersions") && json["gameVersions"].isArray()) {
        QJsonArray versionsArray = json["gameVersions"].toArray();
        for (const QJsonValue& version : versionsArray) {
            mod.gameVersions.append(version.toString());
        }
    }

    // Dependencies
    if (json.contains("addons") && json["addons"].isArray()) {
        QJsonArray addonsArray = json["addons"].toArray();
        for (const QJsonValue& addonValue : addonsArray) {
            QJsonObject addonObj = addonValue.toObject();
            Dependancies dependency;
            dependency.path = addonObj["path"].toString();
            dependency.addOnVersion = addonObj["addOnVersion"].toString();
            dependency.apiVersion = addonObj["apiVersion"].toString();
            dependency.library = addonObj["library"].toBool();
            // extend this to parse optional/required dependencies
            mod.addons.append(dependency);
        }
    }

    mod.isInstalled = json["isInstalled"].toBool(true);
    mod.installPath = json["installPath"].toString();

    return mod;
}