#include "startingwindow.h"
#include "logger.h"
#include "pathing.h"

#include <QApplication>
#include <QScreen>
#include <QStyle>
#include <QScrollBar>
#include <QFont>

StartingWindow::StartingWindow(QWidget *parent) 
    : QMainWindow(parent), httpClient(new HttpClient(1, this)) {
    
    setupUi();
    
    // Connect HTTP client signals
    connect(httpClient, &HttpClient::downloadFinished, 
            this, &StartingWindow::onMasterJsonDownloaded);
    connect(httpClient, &HttpClient::downloadFailed, 
            this, &StartingWindow::onMasterJsonDownloadFailed);
    
    // Set filepath for master list of ESO mods
    masterJsonPath = Pathing::getPaths()->getAppDataPath() + "/master.json";
    
    // Try to load from existing file
    // TODO: Download file if there's a new version'
    QFile file(masterJsonPath);
    if (file.exists()) {
        loadMasterJson();
    } else {
        QUrl url("https://api.mmoui.com/v4/game/ESO/filelist.json");
        httpClient->addDownload(url, masterJsonPath);
        statusLabel->setText("Downloading mod catalog...");
    }
}

StartingWindow::~StartingWindow() {}

void StartingWindow::setupUi() {
    // Window config
    setWindowTitle("ESO Mod Manager");
    resize(1000, 600);
    
    // Center
    setGeometry(
        QStyle::alignedRect(
            Qt::LeftToRight,
            Qt::AlignCenter,
            size(),
            qApp->primaryScreen()->availableGeometry()
        )
    );
    
    centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    QHBoxLayout* mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    
    // Left side panel
    contentPanel = new QFrame(centralWidget);
    contentPanel->setObjectName("contentPanel");
    contentPanel->setStyleSheet(
        "QFrame#contentPanel {"
        "    background-color: #f5f5f5;"
        "}"
    );
    
    QVBoxLayout* contentLayout = new QVBoxLayout(contentPanel);
    contentLayout->setContentsMargins(20, 20, 20, 20);
    
    titleLabel = new QLabel("ESO Mod Manager", contentPanel);
    titleLabel->setObjectName("titleLabel");
    titleLabel->setStyleSheet(
        "QLabel#titleLabel {"
        "    color: #212121;"
        "    font-size: 24px;"
        "    font-weight: bold;"
        "}"
    );
    
    statusLabel = new QLabel("Ready", contentPanel);
    statusLabel->setObjectName("statusLabel");
    statusLabel->setStyleSheet(
        "QLabel#statusLabel {"
        "    color: #757575;"
        "    font-size: 14px;"
        "}"
    );
    
    contentLayout->addWidget(titleLabel);
    contentLayout->addWidget(statusLabel);
    contentLayout->addStretch();
    
    // Right side panel
    sidePanel = new QFrame(centralWidget);
    sidePanel->setObjectName("sidePanel");
    sidePanel->setStyleSheet(
        "QFrame#sidePanel {"
        "    background-color: #ffffff;"
        "    border-left: 1px solid #e0e0e0;"
        "}"
    );
    sidePanel->setMinimumWidth(300);
    sidePanel->setMaximumWidth(300);
    
    QVBoxLayout* sidePanelLayout = new QVBoxLayout(sidePanel);
    sidePanelLayout->setContentsMargins(0, 0, 0, 0);
    
    QFrame* panelHeader = new QFrame(sidePanel);
    panelHeader->setObjectName("panelHeader");
    panelHeader->setStyleSheet(
        "QFrame#panelHeader {"
        "    background-color: #4285f4;"
        "    color: white;"
        "    padding: 10px;"
        "}"
    );
    panelHeader->setMinimumHeight(50);
    
    QHBoxLayout* headerLayout = new QHBoxLayout(panelHeader);
    
    QLabel* modListLabel = new QLabel("Available Mods", panelHeader);
    modListLabel->setStyleSheet(
        "color: white;"
        "font-size: 16px;"
        "font-weight: bold;"
    );
    
    refreshButton = new QPushButton(panelHeader);
    refreshButton->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    refreshButton->setStyleSheet(
        "QPushButton {"
        "    border: none;"
        "    background-color: transparent;"
        "    color: white;"
        "}"
        "QPushButton:hover {"
        "    background-color: rgba(255, 255, 255, 0.1);"
        "}"
        "QPushButton:pressed {"
        "    background-color: rgba(255, 255, 255, 0.2);"
        "}"
    );
    refreshButton->setCursor(Qt::PointingHandCursor); // TODO: remove eventually in favor of automatic checks
    connect(refreshButton, &QPushButton::clicked, [this]() {
        statusLabel->setText("Refreshing mod catalog...");
        QUrl url("https://api.mmoui.com/v4/game/ESO/filelist.json");
        httpClient->addDownload(url, masterJsonPath);
    });
    
    headerLayout->addWidget(modListLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(refreshButton);
    
    modListWidget = new QListWidget(sidePanel);
    modListWidget->setObjectName("modListWidget");
    modListWidget->setStyleSheet(
        "QListWidget#modListWidget {"
        "    border: none;"
        "    background-color: #ffffff;"
        "    outline: none;"
        "}"
        "QListWidget::item {"
        "    border-bottom: 1px solid #e0e0e0;"
        "    padding: 10px;"
        "}"
        "QListWidget::item:selected {"
        "    background-color: #e3f2fd;"
        "    color: #1976d2;"
        "}"
        "QListWidget::item:hover:!selected {"
        "    background-color: #f5f5f5;"
        "}"
    );
    modListWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    modListWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    
    // Scrollbar styling
    modListWidget->verticalScrollBar()->setStyleSheet(
        "QScrollBar:vertical {"
        "    border: none;"
        "    background: #f5f5f5;"
        "    width: 8px;"
        "    margin: 0px;"
        "}"
        "QScrollBar::handle:vertical {"
        "    background: #bdbdbd;"
        "    min-height: 20px;"
        "    border-radius: 4px;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "    height: 0px;"
        "}"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {"
        "    background: none;"
        "}"
    );
    
    connect(modListWidget, &QListWidget::itemClicked, this, &StartingWindow::onModClicked);
    
    sidePanelLayout->addWidget(panelHeader);
    sidePanelLayout->addWidget(modListWidget, 1);
    
    mainLayout->addWidget(contentPanel, 1);
    mainLayout->addWidget(sidePanel);
    
    // Set global font
    QFont appFont = QApplication::font();
    appFont.setFamily("Segoe UI");
    QApplication::setFont(appFont);
}

void StartingWindow::loadMasterJson() {
    QFile file(masterJsonPath);
    
    if (!file.open(QIODevice::ReadOnly)) {
        qCWarning(loggerCategory) << "Failed to open master.json file:" << file.errorString();
        statusLabel->setText("Failed to load mod catalog. Try refreshing.");
        return;
    }
    
    QByteArray jsonData = file.readAll();
    file.close();
    
    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(jsonData, &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        qCWarning(loggerCategory) << "Failed to parse master.json:" << parseError.errorString();
        statusLabel->setText("Failed to parse mod catalog. Try refreshing.");
        return;
    }
    
    if (!document.isArray()) {
        qCWarning(loggerCategory) << "master.json is not a JSON array";
        statusLabel->setText("Invalid mod catalog format. Try refreshing.");
        return;
    }
    
    modEntries = document.array();
    populateModList();
    statusLabel->setText(QString("Loaded %1 mods").arg(modEntries.size()));
}

void StartingWindow::populateModList() {
    modListWidget->clear();
    
    for (const QJsonValue &entry : modEntries) {
        QJsonObject modObject = entry.toObject();
        QString title = modObject["title"].toString();
        
        if (!title.isEmpty()) {
            QListWidgetItem* item = new QListWidgetItem(title);
            item->setData(Qt::UserRole, modObject);
            modListWidget->addItem(item);
        }
    }
    
    if (modListWidget->count() == 0) {
        statusLabel->setText("No mods found in catalog");
    }
}

void StartingWindow::onModClicked(QListWidgetItem* item) {
    QJsonObject modObject = item->data(Qt::UserRole).toJsonObject();
    QString title = modObject["title"].toString();
    
    // For now, just update the status label
    statusLabel->setText("Selected: " + title);
    
    // TODO: Show mod details in the content panel
}

void StartingWindow::onMasterJsonDownloaded(const QString& filePath) {
    if (filePath == masterJsonPath) {
        loadMasterJson();
    }
}

void StartingWindow::onMasterJsonDownloadFailed(const QString& filePath, const QString& error) {
    if (filePath == masterJsonPath) {
        statusLabel->setText("Failed to download mod catalog: " + error);
    }
}
