#include "MainWindow.h"
#include "AuthDialog.h"
#include "InstanceListWidget.h"
#include "InstanceEditorDialog.h"
#include "ConsoleWidget.h"
#include "CreateInstanceDialog.h"
#include "SettingsDialog.h"
#include "InstanceDelegate.h"
#include "KeyBindManager.h"
#include "../core/SettingsManager.h"
#include "../instance/InstanceManager.h"
#include "../cache/CacheManager.h"
#include "../java/JavaManager.h"
#include "../launch/VersionManifest.h"
#include "../launch/LaunchEngine.h"
#include "../core/Logger.h"
#include "../auth/AccountManager.h"
#include "../instance/InstanceImporter.h"
#include <QToolBar>
#include <QStatusBar>
#include <QLabel>
#include <QAction>
#include <QMessageBox>
#include <QInputDialog>
#include <QDesktopServices>
#include <QUrl>
#include <QCloseEvent>
#include <QFileDialog>
#include <QProgressBar>
#include <QHBoxLayout>
#include <QPushButton>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include "../core/GlobalEvents.h"
#include "../network/HttpClient.h"

namespace x4 {

MainWindow::MainWindow(InstanceManager* instanceMgr, CacheManager* cacheMgr,
                       JavaManager* javaMgr, VersionManifest* manifest,
                       LaunchEngine* launchEngine,
                       QWidget* parent)
    : QMainWindow(parent)
    , instanceMgr_(instanceMgr)
    , cacheMgr_(cacheMgr)
    , javaMgr_(javaMgr)
    , manifest_(manifest)
    , launchEngine_(launchEngine)
{
    setWindowTitle(QStringLiteral("X4Launcher"));
    setMinimumSize(800, 500);
    resize(1024, 640);

    setupUI();
    setupSidebar();
    setupStatusBar();
    setupKeyBinds();
    connectSignals();

    
    instanceMgr_->loadAll();
    instanceList_->refresh();
    updateStatusBar();
}

void MainWindow::setupUI() {
    auto* centralWidget = new QWidget(this);
    auto* mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    instanceList_ = new InstanceListWidget(instanceMgr_, centralWidget);
    mainLayout->addWidget(instanceList_, 1);

    setCentralWidget(centralWidget);
}

void MainWindow::setupSidebar() {
    auto* centralWidget = this->centralWidget();
    auto* mainLayout = qobject_cast<QHBoxLayout*>(centralWidget->layout());

    auto* sidebar = new QWidget(centralWidget);
    sidebar->setObjectName("Sidebar");
    sidebar->setFixedWidth(160);
    
    // We style the sidebar directly to match the theme (or rely on Theme.cpp, but here we enforce the style)
    sidebar->setStyleSheet("QWidget#Sidebar { background-color: #1a1b26; border-left: 1px solid #292e42; } "
                           "QPushButton { text-align: left; padding: 10px; font-weight: bold; font-size: 13px; background: transparent; border: 1px solid #16161e; color: #a9b1d6; border-radius: 4px; } "
                           "QPushButton:hover { background-color: #292e42; color: #ffffff; border: 1px solid #292e42; }");

    auto* sidebarLayout = new QVBoxLayout(sidebar);
    sidebarLayout->setContentsMargins(8, 16, 8, 16);
    sidebarLayout->setSpacing(8);

    auto* topInfoLayout = new QVBoxLayout();
    sidebarInstanceIcon_ = new QLabel(this);
    sidebarInstanceIcon_->setFixedSize(64, 64);
    sidebarInstanceIcon_->setAlignment(Qt::AlignCenter);
    sidebarInstanceName_ = new QLabel(this);
    sidebarInstanceName_->setAlignment(Qt::AlignCenter);
    sidebarInstanceName_->setStyleSheet(QStringLiteral("font-weight: bold; color: #a9b1d6;"));
    topInfoLayout->addWidget(sidebarInstanceIcon_, 0, Qt::AlignHCenter);
    topInfoLayout->addWidget(sidebarInstanceName_, 0, Qt::AlignHCenter);
    sidebarLayout->addLayout(topInfoLayout);
    sidebarLayout->addSpacing(16);

    auto createBtn = [this](const QString& text, auto slot) {
        auto* btn = new QPushButton(text, this);
        connect(btn, &QPushButton::clicked, this, slot);
        return btn;
    };

    sidebarLayout->addWidget(createBtn(QStringLiteral("➕  Add Instance"), &MainWindow::addInstance));
    
    auto* btnImport = new QPushButton(QStringLiteral("📦  Import"), this);
    connect(btnImport, &QPushButton::clicked, this, [this]() {
        QString zipPath = QFileDialog::getOpenFileName(this,
            QStringLiteral("Import Instance"),
            QDir::homePath(),
            QStringLiteral("Zip Archives (*.zip);;All Files (*)"));
        if (zipPath.isEmpty()) return;

        auto* importer = new InstanceImporter(instanceMgr_, this);
        connect(importer, &InstanceImporter::importProgress, this, [this](const QString& status) {
            statusBar()->showMessage(status);
        });

        auto result = importer->importZip(zipPath);
        importer->deleteLater();

        if (result.isErr()) {
            QMessageBox::warning(this, QStringLiteral("Import Failed"), result.error());
        } else {
            instanceList_->refresh();
            updateStatusBar();
            QMessageBox::information(this, QStringLiteral("Import Successful"),
                QStringLiteral("Instance '%1' imported successfully.").arg(result.value().name));
        }
    });
    sidebarLayout->addWidget(btnImport);
    
    auto* btnLaunch = new QPushButton(QStringLiteral("▶  Launch"), this);
    connect(btnLaunch, &QPushButton::clicked, this, [this]() {
        QString id = instanceList_->selectedInstanceId();
        if (!id.isEmpty()) launchInstance(id);
    });
    sidebarLayout->addWidget(btnLaunch);

    killButton_ = new QPushButton(QStringLiteral("⏹  Kill"), this);
    killButton_->setStyleSheet(QStringLiteral("QPushButton { color: #f7768e; } QPushButton:hover { background-color: #3b2030; color: #f7768e; }"));
    killButton_->setVisible(false);
    connect(killButton_, &QPushButton::clicked, this, [this]() {
        QString id = instanceList_->selectedInstanceId();
        if (!id.isEmpty()) killInstance(id);
    });
    sidebarLayout->addWidget(killButton_);

    auto* btnEdit = new QPushButton(QStringLiteral("✎  Edit"), this);
    connect(btnEdit, &QPushButton::clicked, this, [this]() {
        QString id = instanceList_->selectedInstanceId();
        if (!id.isEmpty()) editInstance(id);
    });
    sidebarLayout->addWidget(btnEdit);

    sidebarLayout->addWidget(createBtn(QStringLiteral("📂  Folder"), &MainWindow::openInstanceFolder));

    sidebarLayout->addStretch();

    auto* bottomAccountLayout = new QHBoxLayout();
    premiumAvatar_ = new QLabel(this);
    premiumAvatar_->setFixedSize(32, 32);
    QPixmap stevePix(QStringLiteral(":/icons/steve_icon.png"));
    premiumAvatar_->setPixmap(stevePix.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    premiumAvatar_->setStyleSheet(QStringLiteral("background-color: #292e42; border-radius: 4px;"));
    accountNameLabel_ = new QLabel(QStringLiteral("Offline"), this);
    accountNameLabel_->setStyleSheet(QStringLiteral("color: #a9b1d6; font-weight: bold;"));
    bottomAccountLayout->addWidget(premiumAvatar_);
    bottomAccountLayout->addWidget(accountNameLabel_);
    sidebarLayout->addLayout(bottomAccountLayout);

    sidebarLayout->addWidget(createBtn(QStringLiteral("👤  Accounts"), &MainWindow::openAccounts));
    sidebarLayout->addWidget(createBtn(QStringLiteral("⚙  Settings"), &MainWindow::openSettings));

    mainLayout->addWidget(sidebar);
}

void MainWindow::setupStatusBar() {
    globalProgressLabel_ = new QLabel();
    globalProgressLabel_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    globalProgressLabel_->setMinimumWidth(150);

    globalProgress_ = new QProgressBar();
    globalProgress_->setRange(0, 100);
    globalProgress_->setValue(0);
    globalProgress_->setTextVisible(false);
    globalProgress_->setMinimumWidth(150);
    globalProgress_->setMaximumWidth(250);
    globalProgress_->hide();

    auto* w = new QWidget();
    auto* lay = new QHBoxLayout(w);
    lay->setContentsMargins(0, 0, 5, 0);
    lay->addWidget(globalProgress_);
    lay->addWidget(globalProgressLabel_);
    
    statusBar()->addWidget(w);

    playtimeLabel_ = new QLabel(this);
    playtimeLabel_->setStyleSheet(QStringLiteral("color: #7aa2f7; font-weight: bold; padding-right: 10px;"));
    statusBar()->addPermanentWidget(playtimeLabel_);
    
    statusBar()->showMessage(QStringLiteral("Ready"));

    
    connect(&GlobalEvents::instance(), &GlobalEvents::downloadStarted, this, [this](const QString& taskName) {
        globalProgress_->show();
        globalProgress_->setValue(0);
        globalProgressLabel_->setText(taskName);
    });

    connect(&GlobalEvents::instance(), &GlobalEvents::downloadProgress, this, [this](int percent, const QString& speedText) {
        if (!globalProgress_->isVisible()) globalProgress_->show();
        globalProgress_->setValue(percent);
        globalProgressLabel_->setText(speedText);
    });

    connect(&GlobalEvents::instance(), &GlobalEvents::downloadFinished, this, [this](bool success, const QString& message) {
        globalProgress_->hide();
        globalProgressLabel_->clear();
        if (!message.isEmpty()) {
            statusBar()->showMessage(message, 5000);
        }
    });
}

void MainWindow::connectSignals() {
    
    connect(instanceList_, &InstanceListWidget::launchRequested,
            this, &MainWindow::launchInstance);
    connect(instanceList_, &InstanceListWidget::editRequested,
            this, &MainWindow::editInstance);
    connect(instanceList_, &InstanceListWidget::deleteRequested,
            this, &MainWindow::deleteInstance);
    connect(instanceList_, &InstanceListWidget::cloneRequested,
            this, &MainWindow::cloneInstance);

    
    connect(launchEngine_, &LaunchEngine::launchProgress, this,
            [this](const QString&, const QString& status) {
                statusBar()->showMessage(status);
            });

    connect(launchEngine_, &LaunchEngine::launchStarted, this,
            [this](const QString& id) {
                instanceList_->instanceDelegate()->setRunning(id, true);
                instanceList_->viewport()->update();
                killButton_->setVisible(true);
                updateStatusBar();
            });

    connect(launchEngine_, &LaunchEngine::launchFailed, this,
            [this](const QString&, const QString&) {
                statusBar()->showMessage(QStringLiteral("Launch failed"), 5000);
            });

    connect(launchEngine_, &LaunchEngine::gameOutput, this,
            [this](const QString& id, const QString& line) {
                auto it = openEditors_.find(id);
                if (it != openEditors_.end()) {
                    (*it)->consoleWidget()->appendLine(line);
                }
            });

    connect(launchEngine_, &LaunchEngine::gameFinished, this,
            [this](const QString& id, int exitCode) {
                instanceList_->instanceDelegate()->setRunning(id, false);
                instanceList_->viewport()->update();
                killButton_->setVisible(false);
                updateStatusBar();

                if (exitCode != 0) {
                    statusBar()->showMessage(
                        QStringLiteral("Game exited with code %1").arg(exitCode), 10000);
                } else {
                    statusBar()->showMessage(QStringLiteral("Game exited"), 5000);
                }

                
                int action = SettingsManager::instance().closeOnLaunch();
                if (action != 0 && isHidden()) {
                    showNormal();
                }
            });

    
    connect(instanceMgr_, &InstanceManager::instanceAdded, this,
            [this](const Instance&) { updateStatusBar(); });
    connect(instanceMgr_, &InstanceManager::instanceRemoved, this,
            [this](const QString&) { updateStatusBar(); });


    connect(instanceList_->selectionModel(), &QItemSelectionModel::currentChanged, this, [this](const QModelIndex& current, const QModelIndex&) {
        if (current.isValid()) {
            QString id = current.data(InstanceDelegate::IdRole).toString();
            auto* inst = instanceMgr_->findById(id);
            if (inst) {
                sidebarInstanceName_->setText(inst->name);
                // Extract playtime
                qint64 pt = inst->playTime;
                qint64 hours = pt / 3600;
                qint64 minutes = (pt % 3600) / 60;
                playtimeLabel_->setText(QStringLiteral("Total playtime: %1h %2m").arg(hours).arg(minutes));

                QString iconKey = inst->iconKey;
                if (iconKey.isEmpty() || iconKey == QStringLiteral("vanilla_icon")) {
                    QPixmap pix(QStringLiteral(":/icons/vanilla_icon.png"));
                    sidebarInstanceIcon_->setPixmap(pix.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                } else {
                    QPixmap pix(QStringLiteral(":/icons/") + iconKey + QStringLiteral(".png"));
                    sidebarInstanceIcon_->setPixmap(pix.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                }
            }
        } else {
            sidebarInstanceName_->clear();
            sidebarInstanceIcon_->clear();
            playtimeLabel_->clear();
        }
    });

    avatarNetwork_ = HttpClient::instance().manager();

    auto updateAccountInfo = [this]() {
        QString activeId = auth::AccountManager::instance().activeAccountId();
        auto account = auth::AccountManager::instance().account(activeId);
        if (account && !account->minecraftUsername.isEmpty()) {
            accountNameLabel_->setText(account->minecraftUsername);
            
            // Fetch skin face from mc-heads.net (more reliable than crafatar)
            if (!account->uuid.isEmpty()) {
                QString avatarUrl = QStringLiteral("https://mc-heads.net/avatar/%1/32").arg(account->uuid);
                QNetworkReply* reply = avatarNetwork_->get(QNetworkRequest(QUrl(avatarUrl)));
                connect(reply, &QNetworkReply::finished, this, [this, reply]() {
                    reply->deleteLater();
                    if (reply->error() == QNetworkReply::NoError) {
                        QPixmap pix;
                        pix.loadFromData(reply->readAll());
                        if (!pix.isNull()) {
                            premiumAvatar_->setPixmap(pix.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                        }
                    }
                });
            }
        } else {
            QString offlineName = SettingsManager::instance().playerName();
            if (offlineName.isEmpty()) offlineName = QStringLiteral("Player");
            accountNameLabel_->setText(offlineName);
            // Reset to steve icon
            QPixmap stevePix(QStringLiteral(":/icons/steve_icon.png"));
            premiumAvatar_->setPixmap(stevePix.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
    };
    connect(&auth::AccountManager::instance(), &auth::AccountManager::accountsChanged, this, updateAccountInfo);
    updateAccountInfo();
}

void MainWindow::addInstance() {
    if (openCreateInstanceDialog_) {
        openCreateInstanceDialog_->raise();
        openCreateInstanceDialog_->activateWindow();
        return;
    }

    auto* dlg = new CreateInstanceDialog(manifest_, this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setModal(true);
    openCreateInstanceDialog_ = dlg;

    connect(dlg, &QDialog::finished, this, [this]() {
        openCreateInstanceDialog_ = nullptr;
    });

    connect(dlg, &QDialog::accepted, this, [this, dlg]() {
        auto result = instanceMgr_->createInstance(
            dlg->instanceName(), dlg->mcVersion(), dlg->loaderType(), dlg->loaderVersion(), dlg->iconKey());

        if (result.isErr()) {
            QMessageBox::warning(this, QStringLiteral("Error"), result.error());
        } else {
            statusBar()->showMessage(
                QStringLiteral("Created instance: %1").arg(result.value().name), 5000);
            instanceList_->refresh();
            updateStatusBar();
        }
    });

    dlg->show();
    dlg->raise();
    dlg->activateWindow();
}

void MainWindow::openAccounts() {
    if (openAuthDialog_) {
        openAuthDialog_->raise();
        openAuthDialog_->activateWindow();
        return;
    }
    auto* dlg = new AuthDialog(this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    openAuthDialog_ = dlg;
    dlg->show();
    dlg->raise();
    dlg->activateWindow();
}

void MainWindow::openSettings() {
    if (openSettingsDialog_) {
        openSettingsDialog_->raise();
        openSettingsDialog_->activateWindow();
        return;
    }
    auto* dlg = new SettingsDialog(javaMgr_, this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    openSettingsDialog_ = dlg;
    dlg->show();
    dlg->raise();
    dlg->activateWindow();
}

void MainWindow::openInstanceFolder() {
    QString id = instanceList_->selectedInstanceId();
    if (id.isEmpty()) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(
            instanceMgr_->instanceDir(QString())));
        return;
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(instanceMgr_->instanceDir(id)));
}

void MainWindow::launchInstance(const QString& id) {
    auto* inst = instanceMgr_->findById(id);
    if (!inst) return;

    if (launchEngine_->isRunning(id)) {
        QMessageBox::information(this, QStringLiteral("Running"),
            QStringLiteral("%1 is already running.").arg(inst->name));
        return;
    }

    statusBar()->showMessage(QStringLiteral("Launching %1...").arg(inst->name));
    launchEngine_->launch(id);

    
    editInstance(id);
    auto it = openEditors_.find(id);
    if (it != openEditors_.end()) {
        (*it)->switchToConsoleTab();
    }

    int closeAction = SettingsManager::instance().closeOnLaunch();
    if (closeAction == 1) {
        showMinimized();
    } else if (closeAction == 2) {
        hide();
    }
}

void MainWindow::killInstance(const QString& id) {
    auto* inst = instanceMgr_->findById(id);
    if (!inst) return;

    if (!launchEngine_->isRunning(id)) {
        QMessageBox::information(this, QStringLiteral("Not Running"),
            QStringLiteral("%1 is not running.").arg(inst->name));
        return;
    }

    auto answer = QMessageBox::warning(this, QStringLiteral("Kill Instance"),
        QStringLiteral("Force kill \"%1\"?\n\nUnsaved progress will be lost.").arg(inst->name),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (answer != QMessageBox::Yes) return;

    launchEngine_->kill(id);
    statusBar()->showMessage(QStringLiteral("%1 killed").arg(inst->name), 5000);
}

void MainWindow::editInstance(const QString& id) {
    
    auto it = openEditors_.find(id);
    if (it != openEditors_.end()) {
        (*it)->raise();
        (*it)->activateWindow();
        return;
    }

    auto* editor = new InstanceEditorDialog(instanceMgr_, javaMgr_, manifest_, id, this);
    editor->setAttribute(Qt::WA_DeleteOnClose);
    openEditors_[id] = editor;

    connect(editor, &QDialog::destroyed, this, [this, id]() {
        openEditors_.remove(id);
    });

    connect(editor, &InstanceEditorDialog::instanceModified, this,
            [this](const QString&) {
                instanceList_->refresh();
            });

    editor->show();
}

void MainWindow::deleteInstance(const QString& id) {
    auto* inst = instanceMgr_->findById(id);
    if (!inst) return;

    auto answer = QMessageBox::question(this, QStringLiteral("Delete Instance"),
        QStringLiteral("Are you sure you want to delete \"%1\"?\n\n"
                       "This will permanently remove all instance files including "
                       "mods, saves, and configurations.")
            .arg(inst->name),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (answer != QMessageBox::Yes) return;

    
    auto it = openEditors_.find(id);
    if (it != openEditors_.end()) {
        (*it)->close();
    }

    auto result = instanceMgr_->deleteInstance(id);
    if (result.isErr()) {
        QMessageBox::warning(this, QStringLiteral("Error"), result.error());
    } else {
        statusBar()->showMessage(QStringLiteral("Instance deleted"), 5000);
    }
}

void MainWindow::cloneInstance(const QString& id) {
    auto* inst = instanceMgr_->findById(id);
    if (!inst) return;

    bool ok;
    QString newName = QInputDialog::getText(this, QStringLiteral("Clone Instance"),
        QStringLiteral("Name for cloned instance:"), QLineEdit::Normal,
        inst->name + QStringLiteral(" (Copy)"), &ok);

    if (!ok || newName.trimmed().isEmpty()) return;

    auto result = instanceMgr_->cloneInstance(id, newName.trimmed());
    if (result.isErr()) {
        QMessageBox::warning(this, QStringLiteral("Error"), result.error());
    } else {
        statusBar()->showMessage(QStringLiteral("Instance cloned"), 5000);
    }
}

void MainWindow::setupKeyBinds() {
    keyBinds_ = new KeyBindManager(this);

    connect(keyBinds_, &KeyBindManager::launchInstanceTriggered, this, [this]() {
        QString id = instanceList_->selectedInstanceId();
        if (!id.isEmpty()) launchInstance(id);
    });

    connect(keyBinds_, &KeyBindManager::editInstanceTriggered, this, [this]() {
        QString id = instanceList_->selectedInstanceId();
        if (!id.isEmpty()) editInstance(id);
    });

    connect(keyBinds_, &KeyBindManager::addInstanceTriggered, this, &MainWindow::addInstance);

    connect(keyBinds_, &KeyBindManager::deleteInstanceTriggered, this, [this]() {
        QString id = instanceList_->selectedInstanceId();
        if (!id.isEmpty()) deleteInstance(id);
    });

    connect(keyBinds_, &KeyBindManager::cloneInstanceTriggered, this, [this]() {
        QString id = instanceList_->selectedInstanceId();
        if (!id.isEmpty()) cloneInstance(id);
    });

    connect(keyBinds_, &KeyBindManager::openInstanceFolderTriggered, this, &MainWindow::openInstanceFolder);
    connect(keyBinds_, &KeyBindManager::globalSettingsTriggered, this, &MainWindow::openSettings);
    connect(keyBinds_, &KeyBindManager::quitTriggered, this, &QMainWindow::close);
}

void MainWindow::updateStatusBar() {
    int total = instanceMgr_->instances().size();
    statusBar()->showMessage(QStringLiteral("%1 instance(s)").arg(total));
}

} 
