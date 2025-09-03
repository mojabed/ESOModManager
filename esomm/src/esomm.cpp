#include "esomm.h"
#include "ui_esomm.h"
#include "manager.h"
#include "logger.h"

ESOMM::ESOMM(QWidget *parent) : QWidget(parent), manager(new Manager(this)), ui(new Ui::ESOMM) {
    ui->setupUi(this);

    setConnections();
    initUI();

    manager->scanInstalledMods();
    manager->loadAvailableMods();
}

ESOMM::~ESOMM(){}

void ESOMM::initUI() {
    setWindowTitle("ESO Mod Manager");
    resize(1000, 600);

    ui->btnManage->setChecked(true);
    ui->stackedWidget->setCurrentIndex(0); // Manage page

    clearModDetails();
    updateStatusText("ready");
}

void ESOMM::updateStatusText(const QString& text) {
    // TODO: Status display
    qCInfo(loggerCategory) << "Status:" << text;
}

void ESOMM::clearModDetails() {
    if (ui->modTitleLabel) {
        ui->modTitleLabel->setText("Select a mod to view details");
    }
    if (ui->modVersionLabel) {
        ui->modVersionLabel->clear();
    }
    if (ui->modAuthorLabel) {
        ui->modAuthorLabel->clear();
    }
    if (ui->modDescriptionLabel) {
        ui->modDescriptionLabel->clear();
    }
    if (ui->updateModButton) {
        ui->updateModButton->setEnabled(false);
    }
    if (ui->uninstallModButton) {
        ui->uninstallModButton->setEnabled(false);
    }
}

void ESOMM::setConnections() {
    // Manager signals to main slots
    connect(manager, &Manager::installedModsChanged,
        this, &ESOMM::onInstalledModsChanged);

    connect(manager, &Manager::availableModsChanged,
        this, &ESOMM::onAvailableModsChanged);

    connect(manager, &Manager::modActionStarted,
        this, &ESOMM::onModActionStarted);

    connect(manager, &Manager::modActionCompleted,
        this, &ESOMM::onModActionCompleted);

    // UI
    connect(ui->btnManage, &QPushButton::clicked,
        this, &ESOMM::onManageClicked);

    connect(ui->btnBrowse, &QPushButton::clicked,
        this, &ESOMM::onBrowseClicked);

    connect(ui->btnSettings, &QPushButton::clicked,
        this, &ESOMM::onSettingsClicked);

    // Connect mod list interactions for browse page
    // connect(ui->availableModsListWidget, &QListWidget::itemClicked,
    //         this, &ESOMM::onAvailableModClicked);

    // Lists
    if (ui->installedModsListWidget) {
        connect(ui->installedModsListWidget, &QListWidget::itemClicked,
            this, &ESOMM::onInstalledModClicked);
    }

    // Action buttons
    if (ui->refreshInstalledButton) {
        connect(ui->refreshInstalledButton, &QPushButton::clicked,
            this, &ESOMM::onRefreshInstalledClicked);
    }
    if (ui->updateModButton) {
        connect(ui->updateModButton, &QPushButton::clicked,
            this, &ESOMM::onUpdateModClicked);
    }
    if (ui->uninstallModButton) {
        connect(ui->uninstallModButton, &QPushButton::clicked,
            this, &ESOMM::onUninstallModClicked);
    }
}

void ESOMM::onManageClicked() {
    ui->stackedWidget->setCurrentIndex(0); // Manage page
}

void ESOMM::onBrowseClicked() {
    // add a Browse page
    // ui->stackedWidget->setCurrentIndex(1); // Browse page
    qCInfo(loggerCategory) << "Browse clicked - page not implemented yet";
}

void ESOMM::onSettingsClicked() {
    // add a Settings page
    // ui->stackedWidget->setCurrentIndex(2); // Settings page
    qCInfo(loggerCategory) << "Settings clicked - page not implemented yet";
}

void ESOMM::onInstalledModsChanged() {
    qCInfo(loggerCategory) << "Installed mods changed, updating UI";
    updateInstalledModsList();
}

void ESOMM::onAvailableModsChanged() {
    qCInfo(loggerCategory) << "Available mods changed, updating UI";
    // Update available mods list in Browse page
    // updateAvailableModsList();
}

void ESOMM::onModActionStarted(const QString& action, const QString& modTitle) {
    updateStatusText(QString("Starting %1 for %2...").arg(action, modTitle));

    // show a progress indicator here
    // progressBar->setVisible(true);
}

void ESOMM::onModActionCompleted(const QString& action, const QString& modTitle, bool success) {
    if (success) {
        updateStatusText(QString("%1 completed successfully for %2").arg(action, modTitle));
    } else {
        updateStatusText(QString("%1 failed for %2").arg(action, modTitle));
    }

    // Hide progress indicator
    // progressBar->setVisible(false);

    // Refresh the appropriate list
    if (action == "install" || action == "uninstall" || action == "update") {
        manager->scanInstalledMods(); // This will trigger onInstalledModsChanged
    }
}

void ESOMM::onInstalledModClicked(QListWidgetItem* item) {
    if (!item) return;

    QString modId = item->data(Qt::UserRole).toString();
    ModInfo* mod = manager->getInstalledMod(modId);

    if (mod) {
        displayModDetails(*mod);
    }
}

void ESOMM::onRefreshInstalledClicked() {
    updateStatusText("Refreshing installed mods...");
    manager->scanInstalledMods();
}

void ESOMM::onUpdateModClicked() {
    if (selectedModId.isEmpty()) return;

    manager->updateMod(selectedModId);
}

void ESOMM::onUninstallModClicked() {
    if (selectedModId.isEmpty()) return;

    // confirmation dialog
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Confirm Uninstall",
        QString("Are you sure you want to uninstall this mod?"),
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        manager->uninstallMod(selectedModId);
    }
}

void ESOMM::updateInstalledModsList() {
    if (!ui->installedModsListWidget) return;

    ui->installedModsListWidget->clear();

    QList<ModInfo> installedMods = manager->getInstalledMods();

    for (const ModInfo& mod : installedMods) {
        QListWidgetItem* item = new QListWidgetItem(mod.title);
        item->setData(Qt::UserRole, mod.id); // Store mod ID for later use

        // add more visual information
        if (mod.hasUpdate) {
            item->setIcon(style()->standardIcon(QStyle::SP_ArrowUp)); // Update available icon
            item->setToolTip("Update available");
        }

        ui->installedModsListWidget->addItem(item);
    }

    clearModDetails(); // Clear selection when list updates
    selectedModId.clear();

    updateStatusText(QString("Found %1 installed mods").arg(installedMods.size()));
}

void ESOMM::displayModDetails(const ModInfo& mod) {
    selectedModId = mod.id;

    if (ui->modTitleLabel) {
        ui->modTitleLabel->setText(mod.title);
    }

    if (ui->modVersionLabel) {
        ui->modVersionLabel->setText(QString("Version: %1").arg(mod.version));
    }

    if (ui->modAuthorLabel) {
        ui->modAuthorLabel->setText(QString("Author: %1").arg(mod.author));
    }

    if (ui->modDescriptionLabel) {
        // TODO
        ui->modDescriptionLabel->setText(mod.checksum);
    }

    if (ui->updateModButton) {
        ui->updateModButton->setEnabled(mod.hasUpdate);
    }

    if (ui->uninstallModButton) {
        ui->uninstallModButton->setEnabled(true);
    }
}