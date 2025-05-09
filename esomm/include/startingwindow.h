#pragma once

#include "http_client.h"

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLabel>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QFile>
#include <QtWidgets/QFrame>

class StartingWindow : public QMainWindow {
    Q_OBJECT

public:
    StartingWindow(QWidget *parent = nullptr);
    ~StartingWindow();

    void setupUi();
    void loadMasterJson();
    void populateModList();

private slots:
    void onModClicked(QListWidgetItem* item);
    void onMasterJsonDownloaded(const QString& filePath);
    void onMasterJsonDownloadFailed(const QString& filePath, const QString& error);

private:
    // UI Components
    QWidget* centralWidget;
    QFrame* sidePanel;
    QFrame* contentPanel;
    QListWidget* modListWidget;
    QLabel* titleLabel;
    QPushButton* refreshButton;
    QLabel* statusLabel;
    
    // Data
    QJsonArray modEntries;
    HttpClient* httpClient;
    QString masterJsonPath;
};
