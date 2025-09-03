#pragma once

#include <QtWidgets/QWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QApplication>
#include <QScreen>
#include <QStyle>

class Manager;
struct ModInfo;

QT_BEGIN_NAMESPACE
namespace Ui { class ESOMM; }
QT_END_NAMESPACE

class ESOMM : public QWidget {
    Q_OBJECT

public:
    explicit ESOMM(QWidget *parent = nullptr);
    ~ESOMM();

private:
    Ui::ESOMM *ui;
    Manager* manager;
    QString selectedModId;

    void setConnections();
    void initUI();
    void displayModDetails(const ModInfo& mod);
    void updateStatusText(const QString& text);
    void updateInstalledModsList();
    void clearModDetails();

private slots:
    // UI handling
    void onInstalledModClicked(QListWidgetItem* item);
    void onRefreshInstalledClicked();
    void onUpdateModClicked();
    void onUninstallModClicked();
    void onManageClicked();
    void onBrowseClicked();
    void onSettingsClicked();

    // Manager
    void onInstalledModsChanged();
    void onAvailableModsChanged();
    void onModActionStarted(const QString& action, const QString& modTitle);
    void onModActionCompleted(const QString& action, const QString& modTitle, bool success);
};
