#include "InstanceEditorDialog.h"
#include "ConsoleWidget.h"
#include "../auth/AccountManager.h"
#include "../instance/InstanceManager.h"
#include "../instance/InstanceConfig.h"
#include "../core/SettingsManager.h"
#include "../java/JavaManager.h"
#include "../launch/VersionManifest.h"
#include "../launch/LaunchEngine.h"
#include "../launch/MetaAPI.h"
#include "ModrinthBrowserDialog.h"
#include "../network/HttpClient.h"
#include <QMenu>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QTabWidget>
#include <QShortcut>
#include <QListWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QLabel>
#include <QGroupBox>
#include <QCheckBox>
#include <QFileInfo>
#include <QDir>
#include <QDirIterator>
#include <QDesktopServices>
#include <QUrl>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QtConcurrent>
#include <QCryptographicHash>
#include <QNetworkReply>

#include "../modrinth/ModrinthAPI.h"

namespace x4 {

InstanceEditorDialog::InstanceEditorDialog(InstanceManager* mgr, JavaManager* javaMgr,
                                           VersionManifest* manifest, LaunchEngine* launchEngine,
                                           const QString& instanceId,
                                           QWidget* parent)
    : QDialog(parent), mgr_(mgr), javaMgr_(javaMgr), manifest_(manifest), launchEngine_(launchEngine), instanceId_(instanceId)
{
    const auto* inst = mgr_->findById(instanceId);
    if (!inst) {
        close();
        return;
    }

    metaApi_ = new MetaAPI(this);
    modrinthApi_ = new ModrinthAPI(this);

    connect(metaApi_, &MetaAPI::indexFinished, this, [this](const QString&, const QByteArray& data) {
        auto doc = QJsonDocument::fromJson(data);
        auto versions = doc.object()[QStringLiteral("versions")].toArray();
        QString selectedMC = versionCombo_->currentText();

        loaderVersionCombo_->clear();
        loaderVersionCombo_->addItem(QStringLiteral("(latest compatible)"), QString());

        for (const auto& item : versions) {
            auto vobj = item.toObject();
            auto reqs = vobj[QStringLiteral("requires")].toArray();
            bool compatible = false;

            if (reqs.isEmpty()) {
                compatible = true;
            } else {
                bool hasMCReq = false;
                for (const auto& reqItem : reqs) {
                    auto r = reqItem.toObject();
                    if (r[QStringLiteral("uid")].toString() == QStringLiteral("net.minecraft")) {
                        hasMCReq = true;
                        QString eq = r[QStringLiteral("equals")].toString();
                        if (eq == selectedMC || eq.isEmpty())
                            compatible = true;
                    }
                }
                if (!hasMCReq)
                    compatible = true;
            }

            if (compatible) {
                QString ver = vobj[QStringLiteral("version")].toString();
                bool rec = vobj[QStringLiteral("recommended")].toBool();
                loaderVersionCombo_->addItem(rec ? ver + QStringLiteral(" ★") : ver, ver);
            }
        }

        const auto* inst = mgr_->findById(instanceId_);
        if (inst && !inst->loaderVersion.isEmpty()) {
            int idx = loaderVersionCombo_->findData(inst->loaderVersion);
            if (idx >= 0) loaderVersionCombo_->setCurrentIndex(idx);
        }

        loaderVersionCombo_->setEnabled(loaderVersionCombo_->count() > 1);
        loaderStatusLabel_->setText(
            loaderVersionCombo_->count() > 1
            ? QStringLiteral("%1 versions available").arg(loaderVersionCombo_->count() - 1)
            : QStringLiteral("No compatible versions"));
    });

    connect(metaApi_, &MetaAPI::indexFailed, this, [this](const QString&, const QString& err) {
        loaderStatusLabel_->setText(QStringLiteral("Error: %1").arg(err));
    });

    setWindowTitle(QStringLiteral("Edit — %1").arg(inst->name));
    setMinimumSize(720, 520);
    resize(820, 600);

    setupUI();
    loadMetadataCache();
    loadData();

    // Update UI when global settings change (hot-reload)
    connect(&SettingsManager::instance(), &SettingsManager::settingChanged, this,
            [this](const QString& key) {
                if (key == QStringLiteral("java/path")) {
                    javaPathEdit_->setPlaceholderText(QStringLiteral("(use global default: %1)")
                        .arg(SettingsManager::instance().defaultJavaPath().isEmpty() ? QStringLiteral("auto-detect") : SettingsManager::instance().defaultJavaPath()));
                }
                if (key == QStringLiteral("minecraft/fullscreen")) {
                    auto config = mgr_->loadConfig(instanceId_);
                    if (!config.hasOverride(QStringLiteral("fullscreen"))) {
                        fullscreenCheck_->setChecked(SettingsManager::instance().fullscreen());
                    }
                }
            });
}

void InstanceEditorDialog::setupUI() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    tabs_ = new QTabWidget(this);

    tabs_->addTab(createOverviewTab(), QStringLiteral("Overview"));
    tabs_->addTab(createModsTab(), QStringLiteral("Mods"));
    tabs_->addTab(createWorldsTab(), QStringLiteral("Worlds"));
    tabs_->addTab(createResourcePacksTab(), QStringLiteral("Resource Packs"));
    tabs_->addTab(createShaderPacksTab(), QStringLiteral("Shader Packs"));
    tabs_->addTab(createSettingsTab(), QStringLiteral("Settings"));
    tabs_->addTab(createConsoleTab(), QStringLiteral("Console"));

    layout->addWidget(tabs_);

    connect(tabs_, &QTabWidget::currentChanged, this, [this](int index) {
        if (loadedTabs_.contains(index)) return;
        loadedTabs_.insert(index);
        QString title = tabs_->tabText(index);
        if (title == QStringLiteral("Mods")) refreshModList();
        else if (title == QStringLiteral("Worlds")) refreshWorldList();
        else if (title == QStringLiteral("Resource Packs")) refreshResourcePackList();
        else if (title == QStringLiteral("Shader Packs")) refreshShaderPackList();
    });
    loadedTabs_.insert(0); // Overview is default

     auto* btnLayout = new QHBoxLayout();
     btnLayout->setContentsMargins(12, 8, 12, 8);

     auto* launchBtn = new QPushButton(QStringLiteral("▶  Launch"), this);
     launchBtn->setStyleSheet(QStringLiteral("QPushButton { color: #9ece6a; font-weight: bold; } QPushButton:hover { background-color: #1a2e1a; }"));
     connect(launchBtn, &QPushButton::clicked, this, [this]() {
         emit launchRequested(instanceId_);
     });
     btnLayout->addWidget(launchBtn);

     auto* killBtn = new QPushButton(QStringLiteral("⏹  Kill"), this);
     killBtn->setStyleSheet(QStringLiteral("QPushButton { color: #f7768e; font-weight: bold; } QPushButton:hover { background-color: #3b2030; }"));
     killBtn->setVisible(launchEngine_ && launchEngine_->isRunning(instanceId_));
     connect(killBtn, &QPushButton::clicked, this, [this]() {
         emit killRequested(instanceId_);
     });
     btnLayout->addWidget(killBtn);

     if (launchEngine_) {
         connect(launchEngine_, &LaunchEngine::launchStarted, this, [killBtn, this](const QString& id) {
             if (id == instanceId_) killBtn->setVisible(true);
         });
         connect(launchEngine_, &LaunchEngine::gameFinished, this, [killBtn, this](const QString& id, int) {
             if (id == instanceId_) killBtn->setVisible(false);
         });
     }

     btnLayout->addStretch();

     auto* saveBtn = new QPushButton(QStringLiteral("Save"), this);
     connect(saveBtn, &QPushButton::clicked, this, [this]() {
         saveData();
     });
     btnLayout->addWidget(saveBtn);

     auto* closeBtn = new QPushButton(QStringLiteral("Close"), this);
     connect(closeBtn, &QPushButton::clicked, this, &QDialog::close);
     btnLayout->addWidget(closeBtn);

     // Shortcut Ctrl+S to save
     auto* saveShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_S), this);
     connect(saveShortcut, &QShortcut::activated, this, [this]() { saveData(); });

     layout->addLayout(btnLayout);
}

QWidget* InstanceEditorDialog::createOverviewTab() {
    auto* widget = new QWidget();
    auto* layout = new QFormLayout(widget);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(10);

    nameEdit_ = new QLineEdit(widget);
    layout->addRow(QStringLiteral("Name:"), nameEdit_);

    iconCombo_ = new QComboBox(widget);
    iconCombo_->setIconSize(QSize(24, 24));
    iconCombo_->addItem(QIcon(QStringLiteral(":/icons/vanilla_icon.png")), QStringLiteral("Vanilla"), QStringLiteral("vanilla_icon"));
    iconCombo_->addItem(QIcon(QStringLiteral(":/icons/steve_icon.png")), QStringLiteral("Steve"), QStringLiteral("steve_icon"));
    iconCombo_->addItem(QIcon(QStringLiteral(":/icons/forge_icon.png")), QStringLiteral("Forge"), QStringLiteral("forge_icon"));
    iconCombo_->addItem(QIcon(QStringLiteral(":/icons/fabric_icon.png")), QStringLiteral("Fabric"), QStringLiteral("fabric_icon"));
    iconCombo_->addItem(QIcon(QStringLiteral(":/icons/creeper_icon.png")), QStringLiteral("Creeper"), QStringLiteral("creeper_icon"));
    iconCombo_->addItem(QIcon(QStringLiteral(":/icons/chicken_icon.png")), QStringLiteral("Chicken"), QStringLiteral("chicken_icon"));
    iconCombo_->addItem(QIcon(QStringLiteral(":/icons/goat_icon.png")), QStringLiteral("Goat"), QStringLiteral("goat_icon"));
    iconCombo_->addItem(QIcon(QStringLiteral(":/icons/cat_icon.png")), QStringLiteral("Cat"), QStringLiteral("cat_icon"));
    layout->addRow(QStringLiteral("Icon:"), iconCombo_);

    versionCombo_ = new QComboBox(widget);
    versionCombo_->setEditable(true);
    versionCombo_->setInsertPolicy(QComboBox::NoInsert);
    versionCombo_->setMaxVisibleItems(15);
    layout->addRow(QStringLiteral("Minecraft:"), versionCombo_);

    showSnapshots_ = new QCheckBox(QStringLiteral("Show snapshots"), widget);
    layout->addRow(QString(), showSnapshots_);

    loaderCombo_ = new QComboBox(widget);
    loaderCombo_->addItem(QStringLiteral("Vanilla"), static_cast<int>(ModLoaderType::Vanilla));
    loaderCombo_->addItem(QStringLiteral("Fabric"), static_cast<int>(ModLoaderType::Fabric));
    loaderCombo_->addItem(QStringLiteral("Forge"), static_cast<int>(ModLoaderType::Forge));
    loaderCombo_->addItem(QStringLiteral("NeoForge"), static_cast<int>(ModLoaderType::NeoForge));
    loaderCombo_->addItem(QStringLiteral("Quilt"), static_cast<int>(ModLoaderType::Quilt));
    layout->addRow(QStringLiteral("Mod Loader:"), loaderCombo_);

    loaderVersionCombo_ = new QComboBox(widget);
    loaderVersionCombo_->setEnabled(false);
    loaderVersionCombo_->setMaxVisibleItems(20);
    layout->addRow(QStringLiteral("Loader Version:"), loaderVersionCombo_);

    loaderStatusLabel_ = new QLabel(widget);
    loaderStatusLabel_->setStyleSheet(QStringLiteral("color: gray; font-size: 11px;"));
    layout->addRow(QString(), loaderStatusLabel_);

    auto* folderBtn = new QPushButton(QStringLiteral("Open Instance Folder"), widget);
    connect(folderBtn, &QPushButton::clicked, this, [this]() {
        QDesktopServices::openUrl(QUrl::fromLocalFile(mgr_->instanceDir(instanceId_)));
    });
    layout->addRow(QString(), folderBtn);



    auto populateVers = [this]() {
        QString current = versionCombo_->currentText();
        versionCombo_->clear();
        bool snapshots = showSnapshots_->isChecked();
        for (const auto& ver : manifest_->versions()) {
            if (ver.type == QStringLiteral("release")) {
                versionCombo_->addItem(ver.id);
            } else if (snapshots && ver.type == QStringLiteral("snapshot")) {
                versionCombo_->addItem(ver.id);
            }
        }
        if (!current.isEmpty()) {
            int idx = versionCombo_->findText(current);
            if (idx >= 0) versionCombo_->setCurrentIndex(idx);
            else versionCombo_->setEditText(current);
        }
    };

    connect(showSnapshots_, &QCheckBox::toggled, this, populateVers);
    populateVers();

    connect(loaderCombo_, &QComboBox::currentIndexChanged, this, [this]() {
        fetchLoaderVersions();
    });
    connect(versionCombo_, &QComboBox::currentTextChanged, this, [this]() {
        fetchLoaderVersions();
    });

    return widget;
}

void InstanceEditorDialog::fetchLoaderVersions() {
    auto loader = static_cast<ModLoaderType>(loaderCombo_->currentData().toInt());
    loaderVersionCombo_->clear();
    loaderVersionCombo_->setEnabled(false);
    loaderStatusLabel_->clear();

    if (loader == ModLoaderType::Vanilla) {
        loaderStatusLabel_->setText(QStringLiteral("No loader needed"));
        return;
    }

    QString uid;
    switch (loader) {
        case ModLoaderType::Fabric:  uid = QStringLiteral("net.fabricmc.fabric-loader"); break;
        case ModLoaderType::Forge:   uid = QStringLiteral("net.minecraftforge"); break;
        case ModLoaderType::NeoForge: uid = QStringLiteral("net.neoforged"); break;
        case ModLoaderType::Quilt:   uid = QStringLiteral("org.quiltmc.quilt-loader"); break;
        default: return;
    }

    loaderStatusLabel_->setText(QStringLiteral("Loading versions..."));
    metaApi_->getPackageIndex(uid);
}

QWidget* InstanceEditorDialog::createModsTab() {
    auto* widget = new QWidget();
    auto* mainLayout = new QHBoxLayout(widget);
    mainLayout->setContentsMargins(12, 12, 12, 12);

    auto* sidebarLayout = new QVBoxLayout();
    
    auto* downloadBtn = new QPushButton(QStringLiteral("⬇  Download Mods"), widget);
    connect(downloadBtn, &QPushButton::clicked, this, [this]() {
        if (modrinthBrowser_) {
            modrinthBrowser_->setProjectType(QStringLiteral("mod"));
            modrinthBrowser_->raise();
            modrinthBrowser_->activateWindow();
            return;
        }
        modrinthBrowser_ = new ModrinthBrowserDialog(instanceId_, mgr_, QStringLiteral("mod"), this);
        modrinthBrowser_->setAttribute(Qt::WA_DeleteOnClose);
        connect(modrinthBrowser_, &QDialog::destroyed, this, [this]() {
            modrinthBrowser_ = nullptr;
            refreshModList();
        });
        modrinthBrowser_->show();
    });
    
    auto* updateBtn = new QPushButton(QStringLiteral("🔄 Update Selected"), widget);
    connect(updateBtn, &QPushButton::clicked, this, &InstanceEditorDialog::updateSelectedItems);
    
    auto* toggleBtn = new QPushButton(QStringLiteral("⊘ Toggle Selected"), widget);
    connect(toggleBtn, &QPushButton::clicked, this, &InstanceEditorDialog::toggleSelectedItems);
    
    auto* deleteBtn = new QPushButton(QStringLiteral("🗑 Delete Selected"), widget);
    connect(deleteBtn, &QPushButton::clicked, this, &InstanceEditorDialog::deleteSelectedItems);
    
    auto* openBtn = new QPushButton(QStringLiteral("📂 Open Folder"), widget);
    connect(openBtn, &QPushButton::clicked, this, [this]() {
        QString dir = mgr_->minecraftDir(instanceId_) + QStringLiteral("/mods");
        QDir().mkpath(dir);
        QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
    });
    
    sidebarLayout->addWidget(downloadBtn);
    sidebarLayout->addWidget(updateBtn);
    sidebarLayout->addWidget(toggleBtn);
    sidebarLayout->addWidget(deleteBtn);
    sidebarLayout->addStretch();
    sidebarLayout->addWidget(openBtn);
    
    modList_ = new QListWidget(widget);
    modList_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    modList_->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(modList_, &QListWidget::customContextMenuRequested, this, &InstanceEditorDialog::showItemContextMenu);

    mainLayout->addLayout(sidebarLayout, 1);
    mainLayout->addWidget(modList_, 4);

    return widget;
}

QWidget* InstanceEditorDialog::createWorldsTab() {
    auto* widget = new QWidget();
    auto* layout = new QVBoxLayout(widget);
    layout->setContentsMargins(12, 12, 12, 12);

    worldList_ = new QListWidget(widget);
    layout->addWidget(worldList_);

    auto* openBtn = new QPushButton(QStringLiteral("Open Saves Folder"), widget);
    connect(openBtn, &QPushButton::clicked, this, [this]() {
        QString dir = mgr_->minecraftDir(instanceId_) + QStringLiteral("/saves");
        QDir().mkpath(dir);
        QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
    });

    auto* btnLayout = new QHBoxLayout();
    btnLayout->addWidget(openBtn);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    return widget;
}

QWidget* InstanceEditorDialog::createResourcePacksTab() {
    auto* widget = new QWidget();
    auto* mainLayout = new QHBoxLayout(widget);
    mainLayout->setContentsMargins(12, 12, 12, 12);

    auto* sidebarLayout = new QVBoxLayout();
    
    auto* downloadBtn = new QPushButton(QStringLiteral("⬇  Download Packs"), widget);
    connect(downloadBtn, &QPushButton::clicked, this, [this]() {
        if (modrinthBrowser_) {
            modrinthBrowser_->setProjectType(QStringLiteral("resourcepack"));
            modrinthBrowser_->raise();
            modrinthBrowser_->activateWindow();
            return;
        }
        modrinthBrowser_ = new ModrinthBrowserDialog(instanceId_, mgr_, QStringLiteral("resourcepack"), this);
        modrinthBrowser_->setAttribute(Qt::WA_DeleteOnClose);
        connect(modrinthBrowser_, &QDialog::destroyed, this, [this]() {
            modrinthBrowser_ = nullptr;
            refreshResourcePackList();
        });
        modrinthBrowser_->show();
    });
    
    auto* updateBtn = new QPushButton(QStringLiteral("🔄 Update Selected"), widget);
    connect(updateBtn, &QPushButton::clicked, this, &InstanceEditorDialog::updateSelectedItems);
    
    auto* toggleBtn = new QPushButton(QStringLiteral("⊘ Toggle Selected"), widget);
    connect(toggleBtn, &QPushButton::clicked, this, &InstanceEditorDialog::toggleSelectedItems);
    
    auto* deleteBtn = new QPushButton(QStringLiteral("🗑 Delete Selected"), widget);
    connect(deleteBtn, &QPushButton::clicked, this, &InstanceEditorDialog::deleteSelectedItems);
    
    auto* openBtn = new QPushButton(QStringLiteral("📂 Open Folder"), widget);
    connect(openBtn, &QPushButton::clicked, this, [this]() {
        QString dir = mgr_->minecraftDir(instanceId_) + QStringLiteral("/resourcepacks");
        QDir().mkpath(dir);
        QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
    });
    
    sidebarLayout->addWidget(downloadBtn);
    sidebarLayout->addWidget(updateBtn);
    sidebarLayout->addWidget(toggleBtn);
    sidebarLayout->addWidget(deleteBtn);
    sidebarLayout->addStretch();
    sidebarLayout->addWidget(openBtn);
    
    resourcePackList_ = new QListWidget(widget);
    resourcePackList_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    resourcePackList_->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(resourcePackList_, &QListWidget::customContextMenuRequested, this, &InstanceEditorDialog::showItemContextMenu);

    mainLayout->addLayout(sidebarLayout, 1);
    mainLayout->addWidget(resourcePackList_, 4);

    return widget;
}

QWidget* InstanceEditorDialog::createShaderPacksTab() {
    auto* widget = new QWidget();
    auto* mainLayout = new QHBoxLayout(widget);
    mainLayout->setContentsMargins(12, 12, 12, 12);

    auto* sidebarLayout = new QVBoxLayout();
    
    auto* downloadBtn = new QPushButton(QStringLiteral("⬇  Download Shaders"), widget);
    connect(downloadBtn, &QPushButton::clicked, this, [this]() {
        if (modrinthBrowser_) {
            modrinthBrowser_->setProjectType(QStringLiteral("shader"));
            modrinthBrowser_->raise();
            modrinthBrowser_->activateWindow();
            return;
        }
        modrinthBrowser_ = new ModrinthBrowserDialog(instanceId_, mgr_, QStringLiteral("shader"), this);
        modrinthBrowser_->setAttribute(Qt::WA_DeleteOnClose);
        connect(modrinthBrowser_, &QDialog::destroyed, this, [this]() {
            modrinthBrowser_ = nullptr;
            refreshShaderPackList();
        });
        modrinthBrowser_->show();
    });
    
    auto* updateBtn = new QPushButton(QStringLiteral("🔄 Update Selected"), widget);
    connect(updateBtn, &QPushButton::clicked, this, &InstanceEditorDialog::updateSelectedItems);
    
    auto* toggleBtn = new QPushButton(QStringLiteral("⊘ Toggle Selected"), widget);
    connect(toggleBtn, &QPushButton::clicked, this, &InstanceEditorDialog::toggleSelectedItems);
    
    auto* deleteBtn = new QPushButton(QStringLiteral("🗑 Delete Selected"), widget);
    connect(deleteBtn, &QPushButton::clicked, this, &InstanceEditorDialog::deleteSelectedItems);
    
    auto* openBtn = new QPushButton(QStringLiteral("📂 Open Folder"), widget);
    connect(openBtn, &QPushButton::clicked, this, [this]() {
        QString dir = mgr_->minecraftDir(instanceId_) + QStringLiteral("/shaderpacks");
        QDir().mkpath(dir);
        QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
    });
    
    sidebarLayout->addWidget(downloadBtn);
    sidebarLayout->addWidget(updateBtn);
    sidebarLayout->addWidget(toggleBtn);
    sidebarLayout->addWidget(deleteBtn);
    sidebarLayout->addStretch();
    sidebarLayout->addWidget(openBtn);
    
    shaderPackList_ = new QListWidget(widget);
    shaderPackList_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    shaderPackList_->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(shaderPackList_, &QListWidget::customContextMenuRequested, this, &InstanceEditorDialog::showItemContextMenu);

    mainLayout->addLayout(sidebarLayout, 1);
    mainLayout->addWidget(shaderPackList_, 4);

    return widget;
}


QWidget* InstanceEditorDialog::createSettingsTab() {
    auto* widget = new QWidget();
    auto* layout = new QFormLayout(widget);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(10);

    auto& settings = SettingsManager::instance();

    javaPathEdit_ = new QLineEdit(widget);
    javaPathEdit_->setPlaceholderText(QStringLiteral("(use global default: %1)")
        .arg(settings.defaultJavaPath().isEmpty() ? QStringLiteral("auto-detect") : settings.defaultJavaPath()));
    layout->addRow(QStringLiteral("Java Path:"), javaPathEdit_);

    auto* javaGroup = new QGroupBox(QStringLiteral("Detected Installations"), widget);
    auto* javaLayout = new QVBoxLayout(javaGroup);
    javaList_ = new QListWidget(widget);
    javaList_->setMaximumHeight(100);
    javaLayout->addWidget(javaList_);
    layout->addRow(javaGroup);

    auto populateList = [this]() {
        javaList_->clear();
        for (const auto& j : javaMgr_->detectedJavas()) {
            QString text = QStringLiteral("Java %1 (%2) — %3%4")
                .arg(j.major).arg(j.arch, j.path,
                     j.isManaged ? QStringLiteral(" [managed]") : QString());
            auto* item = new QListWidgetItem(text, javaList_);
            item->setData(Qt::UserRole, j.path);
        }
    };
    connect(javaMgr_, &JavaManager::detectionFinished, this, populateList);
    populateList();

    connect(javaList_, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        if (item) {
            javaPathEdit_->setText(item->data(Qt::UserRole).toString());
        }
    });

    auto* memGroup = new QGroupBox(QStringLiteral("Memory"), widget);
    auto* memLayout = new QHBoxLayout(memGroup);

    minMemSpin_ = new QSpinBox(widget);
    minMemSpin_->setRange(256, 32768);
    minMemSpin_->setSuffix(QStringLiteral(" MB"));
    minMemSpin_->setSpecialValueText(QStringLiteral("Global Default"));
    memLayout->addWidget(new QLabel(QStringLiteral("Min:")));
    memLayout->addWidget(minMemSpin_);

    maxMemSpin_ = new QSpinBox(widget);
    maxMemSpin_->setRange(512, 65536);
    maxMemSpin_->setSuffix(QStringLiteral(" MB"));
    maxMemSpin_->setSpecialValueText(QStringLiteral("Global Default"));
    memLayout->addWidget(new QLabel(QStringLiteral("Max:")));
    memLayout->addWidget(maxMemSpin_);

    layout->addRow(memGroup);

    jvmArgsEdit_ = new QLineEdit(widget);
    jvmArgsEdit_->setPlaceholderText(QStringLiteral("(use global default)"));
    layout->addRow(QStringLiteral("JVM Args:"), jvmArgsEdit_);

    gameArgsEdit_ = new QLineEdit(widget);
    gameArgsEdit_->setPlaceholderText(QStringLiteral("(use global default)"));
    layout->addRow(QStringLiteral("Game Args:"), gameArgsEdit_);

    auto* winGroup = new QGroupBox(QStringLiteral("Window Size"), widget);
    auto* winLayout = new QHBoxLayout(winGroup);

    winWidthSpin_ = new QSpinBox(widget);
    winWidthSpin_->setRange(0, 7680);
    winWidthSpin_->setSpecialValueText(QStringLiteral("Default"));
    winLayout->addWidget(new QLabel(QStringLiteral("Width:")));
    winLayout->addWidget(winWidthSpin_);

    winHeightSpin_ = new QSpinBox(widget);
    winHeightSpin_->setRange(0, 4320);
    winHeightSpin_->setSpecialValueText(QStringLiteral("Default"));
    winLayout->addWidget(new QLabel(QStringLiteral("Height:")));
    winLayout->addWidget(winHeightSpin_);

    fullscreenCheck_ = new QCheckBox(QStringLiteral("Fullscreen"), widget);
    winLayout->addWidget(fullscreenCheck_);

    layout->addRow(winGroup);

    return widget;
}

QWidget* InstanceEditorDialog::createConsoleTab() {
    console_ = new ConsoleWidget();
    return console_;
}

void InstanceEditorDialog::switchToConsoleTab() {
    for (int i = 0; i < tabs_->count(); ++i) {
        if (tabs_->tabText(i) == QStringLiteral("Console")) {
            tabs_->setCurrentIndex(i);
            break;
        }
    }
}

void InstanceEditorDialog::loadData() {
    const auto* inst = mgr_->findById(instanceId_);
    if (!inst) return;

    nameEdit_->setText(inst->name);

    int iconIdx = iconCombo_->findData(inst->iconKey);
    if (iconIdx >= 0) iconCombo_->setCurrentIndex(iconIdx);

    int verIdx = versionCombo_->findText(inst->mcVersion);
    if (verIdx >= 0) versionCombo_->setCurrentIndex(verIdx);
    else versionCombo_->setEditText(inst->mcVersion);

    for (int i = 0; i < loaderCombo_->count(); ++i) {
        if (loaderCombo_->itemData(i).toInt() == static_cast<int>(inst->loader)) {
            loaderCombo_->setCurrentIndex(i);
            break;
        }
    }



    auto config = mgr_->loadConfig(instanceId_);
    javaPathEdit_->setText(config.javaPath());
    
    if (config.javaPath().isEmpty() || config.javaPath() == QStringLiteral("auto-detect")) {
        QString detectedPath = javaMgr_->findJavaForMC(inst->mcVersion).path;
        if (!detectedPath.isEmpty()) {
            javaPathEdit_->setPlaceholderText(QStringLiteral("(auto-detect: %1)").arg(detectedPath));
        } else {
            javaPathEdit_->setPlaceholderText(QStringLiteral("(auto-detect: no compatible Java found)"));
        }
    }
    
    minMemSpin_->setValue(config.minMemoryMB());
    maxMemSpin_->setValue(config.maxMemoryMB());
    jvmArgsEdit_->setText(config.jvmArgs());
    gameArgsEdit_->setText(config.gameArgs());
    winWidthSpin_->setValue(config.windowWidth());
    winHeightSpin_->setValue(config.windowHeight());

    if (config.hasOverride(QStringLiteral("fullscreen"))) {
        fullscreenCheck_->setChecked(config.fullscreen());
    } else {
        fullscreenCheck_->setChecked(SettingsManager::instance().fullscreen());
    }

    fetchLoaderVersions();
}



void InstanceEditorDialog::saveData() {
    auto* inst = mgr_->findById(instanceId_);
    if (!inst) return;

    inst->name = nameEdit_->text().trimmed();
    inst->iconKey = iconCombo_->currentData().toString();
    inst->mcVersion = versionCombo_->currentText();
    inst->loader = static_cast<ModLoaderType>(loaderCombo_->currentData().toInt());
    inst->loaderVersion = loaderVersionCombo_->currentData().toString();
    mgr_->saveInstance(*inst);

    auto config = mgr_->loadConfig(instanceId_);
    config.setJavaPath(javaPathEdit_->text().trimmed());
    config.setMinMemoryMB(minMemSpin_->value());
    config.setMaxMemoryMB(maxMemSpin_->value());
    config.setJvmArgs(jvmArgsEdit_->text().trimmed());
    config.setGameArgs(gameArgsEdit_->text().trimmed());
    config.setWindowWidth(winWidthSpin_->value());
    config.setWindowHeight(winHeightSpin_->value());
    config.setFullscreen(fullscreenCheck_->isChecked());
    mgr_->saveConfig(instanceId_, config);

    setWindowTitle(QStringLiteral("Edit — %1").arg(inst->name));
    emit instanceModified(instanceId_);
}

void InstanceEditorDialog::refreshModList() {
    modList_->clear();
    QString dir = mgr_->minecraftDir(instanceId_) + QStringLiteral("/mods");
    populateRichFileList(modList_, dir, {QStringLiteral("*.jar"), QStringLiteral("*.zip"), QStringLiteral("*.disabled")});
}

void InstanceEditorDialog::refreshWorldList() {
    worldList_->clear();
    QString dir = mgr_->minecraftDir(instanceId_) + QStringLiteral("/saves");
    QDir d(dir);
    if (!d.exists()) return;
    for (const auto& entry : d.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        worldList_->addItem(entry);
    }
}

void InstanceEditorDialog::refreshResourcePackList() {
    resourcePackList_->clear();
    QString dir = mgr_->minecraftDir(instanceId_) + QStringLiteral("/resourcepacks");
    populateRichFileList(resourcePackList_, dir, {QStringLiteral("*.zip"), QStringLiteral("*.jar")});
}

void InstanceEditorDialog::refreshShaderPackList() {
    shaderPackList_->clear();
    QString dir = mgr_->minecraftDir(instanceId_) + QStringLiteral("/shaderpacks");
    populateRichFileList(shaderPackList_, dir, {QStringLiteral("*.zip")});
}

void InstanceEditorDialog::populateFileList(QListWidget* list, const QString& dir,
                                             const QStringList& filters) {
    QDir d(dir);
    if (!d.exists()) return;
    for (const auto& fi : d.entryInfoList(filters, QDir::Files)) {
        QString display = fi.fileName();
        if (display.endsWith(QStringLiteral(".disabled")))
            display = QStringLiteral("⊘ ") + display;
        list->addItem(display);
    }
}

void InstanceEditorDialog::populateRichFileList(QListWidget* list, const QString& dir, const QStringList& filters) {
    QDir d(dir);
    if (!d.exists()) return;

    list->clear();
    QStringList files;
    QMap<QString, QListWidgetItem*> fileItems;

    
    for (const auto& fi : d.entryInfoList(filters, QDir::Files)) {
        QString display = fi.fileName();
        bool disabled = display.endsWith(QStringLiteral(".disabled"));
        
        auto* item = new QListWidgetItem();
        item->setSizeHint(QSize(0, 64)); 
        list->addItem(item);

        auto* widget = new QWidget(list);
        auto* layout = new QHBoxLayout(widget);
        layout->setContentsMargins(4, 4, 4, 4);
        
        auto* iconLbl = new QLabel();
        iconLbl->setFixedSize(48, 48);
        iconLbl->setStyleSheet("background-color: #2b2b2b; border-radius: 8px;");
        layout->addWidget(iconLbl);
        
        auto* textLayout = new QVBoxLayout();
        auto* titleLbl = new QLabel(QString("<b>%1</b>").arg(display));
        auto* descLbl = new QLabel(disabled ? "<i>(Disabled)</i> Loading metadata..." : "Loading metadata...");
        
        
        descLbl->setStyleSheet("color: #aaaaaa;");
        descLbl->setWordWrap(true);
        textLayout->addWidget(titleLbl);
        textLayout->addWidget(descLbl);
        textLayout->addStretch();
        layout->addLayout(textLayout);
        
        widget->setLayout(layout);
        list->setItemWidget(item, widget);
        
        files.append(fi.absoluteFilePath());
        
        
        item->setData(Qt::UserRole, fi.absoluteFilePath());
        fileItems.insert(fi.absoluteFilePath(), item);
    }

    if (files.isEmpty()) return;

    
    auto future = QtConcurrent::run([files]() -> QMap<QString, QString> {
        QMap<QString, QString> fileToHash;
        for (const auto& f : files) {
            QFile file(f);
            if (file.open(QIODevice::ReadOnly)) {
                QCryptographicHash hasher(QCryptographicHash::Sha1);
                hasher.addData(&file);
                fileToHash.insert(f, QString::fromLatin1(hasher.result().toHex()));
            }
        }
        return fileToHash;
    });

    auto* watcher = new QFutureWatcher<QMap<QString, QString>>(this);
    connect(watcher, &QFutureWatcher<QMap<QString, QString>>::finished, this, [this, watcher, list, fileItems]() {
        auto fileToHash = watcher->result();
        watcher->deleteLater();
        
        QStringList hasherList;
        QMap<QString, QString> hashToFile;
        for (auto it = fileToHash.begin(); it != fileToHash.end(); ++it) {
            QString hash = it.value();
            QString file = it.key();
            hashToFile.insert(hash, file);

            // Check cache
            if (metadataCache_.contains(hash)) {
                auto ver = metadataCache_.value(hash);
                auto* item = fileItems.value(file);
                if (item) {
                    auto* w = list->itemWidget(item);
                    if (w) {
                        auto* layout = w->layout();
                        auto* iconLbl = qobject_cast<QLabel*>(layout->itemAt(0)->widget());
                        auto* textLayout = qobject_cast<QVBoxLayout*>(layout->itemAt(1)->layout());
                        auto* titleLbl = qobject_cast<QLabel*>(textLayout->itemAt(0)->widget());
                        auto* descLbl = qobject_cast<QLabel*>(textLayout->itemAt(1)->widget());
                        
                        titleLbl->setText(QString("<b>%1</b>").arg(ver.name));
                        descLbl->setText(QString("Project ID: <a href='https://modrinth.com/project/%1'>%1</a> | Version: %2")
                                         .arg(ver.projectId)
                                         .arg(ver.versionNumber));
                        descLbl->setOpenExternalLinks(true);
                        
                        if (!ver.localIconPath.isEmpty() && QFile::exists(ver.localIconPath)) {
                            QPixmap pix(ver.localIconPath);
                            if (!pix.isNull()) {
                                pix = pix.scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                                iconLbl->setPixmap(pix);
                                iconLbl->setStyleSheet("");
                            }
                        } else if (!ver.iconUrl.isEmpty()) {
                            downloadIconForCache(hash, ver.iconUrl, item);
                        }
                    }
                }
            } else {
                hasherList.append(hash);
            }
        }
        
        if (hasherList.isEmpty()) {
            // Prune cache to only include existing files
            QMap<QString, CachedMetadata> newCache;
            for (auto it = fileToHash.begin(); it != fileToHash.end(); ++it) {
                if (metadataCache_.contains(it.value())) {
                    newCache.insert(it.value(), metadataCache_.value(it.value()));
                }
            }
            metadataCache_ = newCache;
            saveMetadataCache();
            return;
        }
        
        modrinthApi_->batchGetByHashes(hasherList);
        
        
        connect(modrinthApi_, &ModrinthAPI::batchHashesFinished, this, [this, list, fileItems, hashToFile, fileToHash, hasherList](const QMap<QString, x4::ModrinthVersion>& res) {
            QStringList projectIds;
            for (auto it = res.begin(); it != res.end(); ++it) {
                if (!it.value().projectId.isEmpty() && !projectIds.contains(it.value().projectId)) {
                    projectIds.append(it.value().projectId);
                }
            }

            if (projectIds.isEmpty()) {
                // If no project IDs, fallback and finish
                for (auto it = res.begin(); it != res.end(); ++it) {
                    QString hash = it.key();
                    auto ver = it.value();
                    QString file = hashToFile.value(hash);
                    CachedMetadata meta;
                    meta.name = ver.name;
                    meta.projectId = ver.projectId.isEmpty() ? "Unknown" : ver.projectId;
                    meta.versionNumber = ver.versionNumber;
                    metadataCache_.insert(hash, meta);
                    // Update UI fallback
                    auto* item = fileItems.value(file);
                    if (item) {
                        auto* w = list->itemWidget(item);
                        if (w) {
                            auto* textLayout = qobject_cast<QVBoxLayout*>(w->layout()->itemAt(1)->layout());
                            auto* titleLbl = qobject_cast<QLabel*>(textLayout->itemAt(0)->widget());
                            auto* descLbl = qobject_cast<QLabel*>(textLayout->itemAt(1)->widget());
                            titleLbl->setText(QString("<b>%1</b>").arg(meta.name));
                            descLbl->setText(QString("Project ID: %1 | Version: %2").arg(meta.projectId).arg(meta.versionNumber));
                        }
                    }
                }
                saveMetadataCache();
                return;
            }

            modrinthApi_->batchGetProjects(projectIds);
            
            connect(modrinthApi_, &ModrinthAPI::batchProjectsFinished, this, [this, res, list, fileItems, hashToFile, fileToHash](const QMap<QString, x4::ModrinthProject>& projects) {
                for (auto it = res.begin(); it != res.end(); ++it) {
                    QString hash = it.key();
                    auto ver = it.value();
                    QString file = hashToFile.value(hash);
                    
                    CachedMetadata meta;
                    if (projects.contains(ver.projectId)) {
                        auto proj = projects.value(ver.projectId);
                        meta.name = proj.title;
                        meta.iconUrl = proj.iconUrl;
                    } else {
                        meta.name = ver.name;
                    }
                    meta.projectId = ver.projectId;
                    meta.versionNumber = ver.versionNumber;
                    
                    metadataCache_.insert(hash, meta);
                    
                    auto* item = fileItems.value(file);
                    if (item) {
                        auto* w = list->itemWidget(item);
                        if (w) {
                            auto* layout = w->layout();
                            auto* textLayout = qobject_cast<QVBoxLayout*>(layout->itemAt(1)->layout());
                            auto* titleLbl = qobject_cast<QLabel*>(textLayout->itemAt(0)->widget());
                            auto* descLbl = qobject_cast<QLabel*>(textLayout->itemAt(1)->widget());
                            
                            titleLbl->setText(QString("<b>%1</b>").arg(meta.name));
                            descLbl->setText(QString("Project ID: <a href='https://modrinth.com/project/%1'>%1</a> | Version: %2")
                                             .arg(meta.projectId)
                                             .arg(meta.versionNumber));
                            descLbl->setOpenExternalLinks(true);
                            
                            if (!meta.iconUrl.isEmpty()) {
                                downloadIconForCache(hash, meta.iconUrl, item);
                            }
                        }
                    }
                }
                
                // Set "Not found" for local-only files
                for (auto* item : fileItems.values()) {
                    QString file = item->data(Qt::UserRole).toString();
                    if (!res.contains(fileToHash.value(file))) {
                        auto* w = list->itemWidget(item);
                        if (w) {
                            auto* textLayout = qobject_cast<QVBoxLayout*>(w->layout()->itemAt(1)->layout());
                            auto* descLbl = qobject_cast<QLabel*>(textLayout->itemAt(1)->widget());
                            descLbl->setText(QStringLiteral("<i>Not found on Modrinth (Local file)</i>"));
                            CachedMetadata meta;
                            meta.name = QFileInfo(file).fileName();
                            meta.versionNumber = "Local";
                            metadataCache_.insert(fileToHash.value(file), meta);
                        }
                    }
                }
                
                // Prune cache
                QMap<QString, CachedMetadata> newCache;
                for (auto it = fileToHash.begin(); it != fileToHash.end(); ++it) {
                    if (metadataCache_.contains(it.value())) {
                        newCache.insert(it.value(), metadataCache_.value(it.value()));
                    }
                }
                metadataCache_ = newCache;
                saveMetadataCache();
            });
        });
    });
}

void InstanceEditorDialog::loadMetadataCache() {
    QString cacheFile = mgr_->instanceDir(instanceId_) + "/.metadata_cache.json";
    QFile f(cacheFile);
    if (f.open(QIODevice::ReadOnly)) {
        auto doc = QJsonDocument::fromJson(f.readAll());
        auto obj = doc.object();
        for (auto it = obj.begin(); it != obj.end(); ++it) {
            auto v = it.value().toObject();
            CachedMetadata m;
            m.name = v["name"].toString();
            m.projectId = v["projectId"].toString();
            m.versionNumber = v["versionNumber"].toString();
            m.iconUrl = v["iconUrl"].toString();
            m.localIconPath = v["localIconPath"].toString();
            metadataCache_.insert(it.key(), m);
        }
    }
}

void InstanceEditorDialog::saveMetadataCache() {
    QString cacheFile = mgr_->instanceDir(instanceId_) + "/.metadata_cache.json";
    QJsonObject obj;
    for (auto it = metadataCache_.begin(); it != metadataCache_.end(); ++it) {
        QJsonObject v;
        v["name"] = it.value().name;
        v["projectId"] = it.value().projectId;
        v["versionNumber"] = it.value().versionNumber;
        v["iconUrl"] = it.value().iconUrl;
        v["localIconPath"] = it.value().localIconPath;
        obj[it.key()] = v;
    }
    QFile f(cacheFile);
    if (f.open(QIODevice::WriteOnly)) {
        f.write(QJsonDocument(obj).toJson(QJsonDocument::Compact));
    }
}

void InstanceEditorDialog::downloadIconForCache(const QString& sha1, const QString& url, QListWidgetItem* item) {
    if (url.isEmpty()) return;
    
    QNetworkRequest req(url);
    req.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
    auto* reply = HttpClient::instance().manager()->get(req);
    
    connect(reply, &QNetworkReply::finished, this, [this, reply, sha1, item]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            QString iconsDir = mgr_->instanceDir(instanceId_) + "/.icons";
            QDir().mkpath(iconsDir);
            QString localPath = iconsDir + "/" + sha1 + ".png";
            
            QFile f(localPath);
            if (f.open(QIODevice::WriteOnly)) {
                QByteArray data = reply->readAll();
                f.write(data);
                f.close();
                
                if (metadataCache_.contains(sha1)) {
                    metadataCache_[sha1].localIconPath = localPath;
                    saveMetadataCache();
                }
                
                if (item) {
                    auto* list = item->listWidget();
                    if (list) {
                        auto* w = list->itemWidget(item);
                        if (w) {
                            auto* layout = w->layout();
                            auto* iconLbl = qobject_cast<QLabel*>(layout->itemAt(0)->widget());
                            if (iconLbl) {
                                QPixmap pix;
                                if (pix.loadFromData(data)) {
                                    pix = pix.scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                                    iconLbl->setPixmap(pix);
                                    iconLbl->setStyleSheet("");
                                }
                            }
                        }
                    }
                }
            }
        }
    });
}

void InstanceEditorDialog::showItemContextMenu(const QPoint& pos) {
    auto* list = qobject_cast<QListWidget*>(sender());
    if (!list) return;

    auto items = list->selectedItems();
    if (items.isEmpty()) return;

    QMenu menu(this);

    auto* toggleAct = menu.addAction("⊘ Toggle Enable/Disable");
    auto* updateAct = menu.addAction("🔄 Update");
    menu.addSeparator();
    auto* deleteAct = menu.addAction("🗑 Delete");
    
    QAction* selectedAct = menu.exec(list->mapToGlobal(pos));
    if (!selectedAct) return;

    if (selectedAct == toggleAct) toggleSelectedItems();
    else if (selectedAct == updateAct) updateSelectedItems();
    else if (selectedAct == deleteAct) deleteSelectedItems();
}

void InstanceEditorDialog::deleteSelectedItems() {
    auto* tabWidget = tabs_->currentWidget();
    QListWidget* list = nullptr;
    if (tabWidget->findChild<QListWidget*>()) {
        list = tabWidget->findChild<QListWidget*>();
    }
    if (!list) return;

    auto items = list->selectedItems();
    if (items.isEmpty()) return;

    if (QMessageBox::question(this, "Confirm Deletion", "Are you sure you want to delete the selected items?") != QMessageBox::Yes) {
        return;
    }

    for (auto* item : items) {
        QString file = item->data(Qt::UserRole).toString();
        if (!file.isEmpty() && QFile::exists(file)) {
            QFile::remove(file);
        }
    }

    if (list == modList_) refreshModList();
    else if (list == resourcePackList_) refreshResourcePackList();
    else if (list == shaderPackList_) refreshShaderPackList();
}

void InstanceEditorDialog::toggleSelectedItems() {
    auto* tabWidget = tabs_->currentWidget();
    QListWidget* list = nullptr;
    if (tabWidget->findChild<QListWidget*>()) {
        list = tabWidget->findChild<QListWidget*>();
    }
    if (!list) return;

    auto items = list->selectedItems();
    for (auto* item : items) {
        QString file = item->data(Qt::UserRole).toString();
        if (file.isEmpty() || !QFile::exists(file)) continue;

        QString newFile;
        if (file.endsWith(".disabled")) {
            newFile = file.left(file.length() - 9);
        } else {
            newFile = file + ".disabled";
        }
        QFile::rename(file, newFile);
    }

    if (list == modList_) refreshModList();
    else if (list == resourcePackList_) refreshResourcePackList();
    else if (list == shaderPackList_) refreshShaderPackList();
}

void InstanceEditorDialog::updateSelectedItems() {
    // Stub prepared for future update logic.
    QMessageBox::information(this, "Update", "Update logic goes here. Need to query Modrinth for the latest versions of the selected items and add them to the Download queue.");
}

void InstanceEditorDialog::openSelectedItemsFolder() {
    // Stub: Optional logic if needed.
}

} 
