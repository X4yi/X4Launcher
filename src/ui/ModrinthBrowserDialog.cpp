#include "ModrinthBrowserDialog.h"
#include "../instance/InstanceManager.h"
#include "../network/DownloadManager.h"
#include "../core/GlobalEvents.h"
#include "../network/HttpClient.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QListWidget>
#include <QListWidgetItem>
#include <QLabel>
#include <QTimer>
#include <QMessageBox>
#include <QProgressDialog>
#include <QSplitter>
#include <QTextBrowser>
#include <QTableWidget>
#include <QHeaderView>
#include <QDir>
#include <QNetworkAccessManager>
#include <QNetworkReply>

namespace x4 {

ModrinthBrowserDialog::ModrinthBrowserDialog(const QString& instanceId, InstanceManager* instanceMgr,
                                               const QString& initialFilter, QWidget* parent)
    : QDialog(parent)
    , instanceId_(instanceId)
    , instanceMgr_(instanceMgr)
{
    
    const auto* inst = instanceMgr_->findById(instanceId_);
    if (inst) {
        currentInstance_ = *inst;
    }

    setWindowTitle(QStringLiteral("Modrinth — %1").arg(currentInstance_.name));
    setMinimumSize(900, 600);
    resize(1000, 700);
    
    api_ = new ModrinthAPI(this);
    imgNetMgr_ = HttpClient::instance().manager();
    searchTimer_ = new QTimer(this);
    searchTimer_->setSingleShot(true);
    searchTimer_->setInterval(400); // 400ms delay to reduce CPU overhead

    connect(api_, &ModrinthAPI::searchFinished, this, &ModrinthBrowserDialog::onSearchFinished);
    connect(api_, &ModrinthAPI::searchFailed, this, &ModrinthBrowserDialog::onSearchFailed);
    connect(api_, &ModrinthAPI::versionsFinished, this, &ModrinthBrowserDialog::onVersionsFinished);
    connect(api_, &ModrinthAPI::versionsFailed, this, &ModrinthBrowserDialog::onVersionsFailed);
    connect(searchTimer_, &QTimer::timeout, this, &ModrinthBrowserDialog::performSearch);

    setupUI();

    // Apply initial filter if provided
    if (!initialFilter.isEmpty()) {
        setProjectType(initialFilter);
    }
}

void ModrinthBrowserDialog::setProjectType(const QString& type) {
    int idx = typeCombo_->findData(type);
    if (idx >= 0) {
        typeCombo_->setCurrentIndex(idx);
    }
}

void ModrinthBrowserDialog::setupUI() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);

    auto* topLayout = new QHBoxLayout();
    
    typeCombo_ = new QComboBox(this);
    typeCombo_->addItem("Mods", "mod");
    typeCombo_->addItem("Resource Packs", "resourcepack");
    typeCombo_->addItem("Shader Packs", "shader");
    typeCombo_->addItem("Datapacks", "datapack");
    topLayout->addWidget(typeCombo_);

    sortCombo_ = new QComboBox(this);
    sortCombo_->addItem("Relevance", "relevance");
    sortCombo_->addItem("Downloads", "downloads");
    sortCombo_->addItem("Newest", "newest");
    sortCombo_->addItem("Updated", "updated");
    topLayout->addWidget(sortCombo_);

    searchEdit_ = new QLineEdit(this);
    searchEdit_->setPlaceholderText("Search Modrinth...");
    topLayout->addWidget(searchEdit_);

    searchBtn_ = new QPushButton("Search", this);
    topLayout->addWidget(searchBtn_);

    layout->addLayout(topLayout);

    splitter_ = new QSplitter(Qt::Horizontal, this);
    
    resultsList_ = new QListWidget(this);
    resultsList_->setSelectionMode(QAbstractItemView::SingleSelection);
    resultsList_->setIconSize(QSize(48, 48));
    splitter_->addWidget(resultsList_);

    auto* rightPanel = new QWidget(this);
    auto* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);

    detailsBrowser_ = new QTextBrowser(this);
    detailsBrowser_->setOpenExternalLinks(true);
    rightLayout->addWidget(detailsBrowser_);

    versionsTable_ = new QTableWidget(this);
    versionsTable_->setColumnCount(3);
    versionsTable_->setHorizontalHeaderLabels({"Version", "Game Versions", "Loaders"});
    versionsTable_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    versionsTable_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    versionsTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    versionsTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    versionsTable_->verticalHeader()->setVisible(false);
    rightLayout->addWidget(versionsTable_);

    auto* rightBottom = new QHBoxLayout();
    rightBottom->addStretch();
    installVersionBtn_ = new QPushButton("Add to Queue", this);
    installVersionBtn_->setEnabled(false);
    rightBottom->addWidget(installVersionBtn_);
    rightLayout->addLayout(rightBottom);

    splitter_->addWidget(rightPanel);
    
    
    auto* queuePanel = new QWidget(this);
    auto* queueLayout = new QVBoxLayout(queuePanel);
    queueLayout->setContentsMargins(0, 0, 0, 0);
    queueLayout->addWidget(new QLabel("Download Queue", this));
    
    queueList_ = new QListWidget(this);
    queueLayout->addWidget(queueList_);
    
    auto* qBtnLayout = new QHBoxLayout();
    clearQueueBtn_ = new QPushButton("Clear", this);
    downloadQueueBtn_ = new QPushButton("Download All", this);
    downloadQueueBtn_->setEnabled(false);
    qBtnLayout->addWidget(clearQueueBtn_);
    qBtnLayout->addWidget(downloadQueueBtn_);
    queueLayout->addLayout(qBtnLayout);

    splitter_->addWidget(queuePanel);

    splitter_->setStretchFactor(0, 1);
    splitter_->setStretchFactor(1, 2);
    splitter_->setStretchFactor(2, 1);

    layout->addWidget(splitter_);

    
     connect(searchBtn_, &QPushButton::clicked, this, &ModrinthBrowserDialog::performSearch);
     connect(searchEdit_, &QLineEdit::returnPressed, this, &ModrinthBrowserDialog::performSearch);
     connect(searchEdit_, &QLineEdit::textChanged, this, &ModrinthBrowserDialog::onSearchTextChanged);
     connect(typeCombo_, &QComboBox::currentIndexChanged, this, &ModrinthBrowserDialog::performSearch);
     connect(sortCombo_, &QComboBox::currentIndexChanged, this, &ModrinthBrowserDialog::performSearch);
    
     connect(resultsList_, &QListWidget::itemSelectionChanged, this, [this]() {
         auto sel = resultsList_->selectedItems();
         if (!sel.isEmpty()) onProjectClicked(sel.first());
     });
     
     // Handle double-click to automatically select and add to queue
     connect(resultsList_, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem* item) {
         onProjectClicked(item);
         // Auto-select latest version and add to queue if we have a project
         if (!currentSelectedProjectId_.isEmpty() && !currentVersions_.isEmpty()) {
             versionsTable_->selectRow(0); // Select first (latest) version
             auto items = versionsTable_->selectedItems();
             if (!items.isEmpty()) {
                 int row = items.first()->row();
                 if (row >= 0 && row < currentVersions_.size()) {
                     const auto& ver = currentVersions_[row];
                     ModrinthFile chosenFile;
                     for (const auto& f : ver.files) {
                         if (f.primary) chosenFile = f;
                     }
                     if (chosenFile.url.isEmpty() && !ver.files.isEmpty()) {
                         chosenFile = ver.files.first();
                     }
                     if (!chosenFile.url.isEmpty()) {
                         addToQueue(ver, chosenFile);
                     }
                 }
             }
         }
     });
    
    connect(versionsTable_, &QTableWidget::itemSelectionChanged, this, [this]() {
        installVersionBtn_->setEnabled(!versionsTable_->selectedItems().isEmpty());
    });
    
    connect(installVersionBtn_, &QPushButton::clicked, this, [this]() {
        auto items = versionsTable_->selectedItems();
        if (items.isEmpty()) return;
        int row = items.first()->row();
        if (row < 0 || row >= currentVersions_.size()) return;
        
        const auto& ver = currentVersions_[row];
        ModrinthFile chosenFile;
        for (const auto& f : ver.files) {
            if (f.primary) chosenFile = f;
        }
        if (chosenFile.url.isEmpty() && !ver.files.isEmpty()) {
            chosenFile = ver.files.first();
        }
        
        if (chosenFile.url.isEmpty()) {
            QMessageBox::warning(this, "Error", "No download file found.");
            return;
        }
        
        addToQueue(ver, chosenFile);
    });
    
    connect(clearQueueBtn_, &QPushButton::clicked, this, [this]() {
        downloadQueue_.clear();
        updateQueueUI();
    });
    
    connect(downloadQueueBtn_, &QPushButton::clicked, this, &ModrinthBrowserDialog::flushQueue);
}

void ModrinthBrowserDialog::onSearchTextChanged() {
    // Restart the timer on each text change
    searchTimer_->start();
}

void ModrinthBrowserDialog::performSearch() {
    resultsList_->clear();
    resultsList_->addItem("Searching...");
    
    QString query = searchEdit_->text().trimmed();
    QString projectType = typeCombo_->currentData().toString();
    QString index = sortCombo_->currentData().toString();
    
    
    QString facets = ModrinthAPI::buildFacets(currentInstance_.mcVersion, currentInstance_.loader, projectType);
    
    api_->searchProjects(query, facets, 30, 0, index);
}

void ModrinthBrowserDialog::onSearchFinished(const x4::ModrinthSearchResult& result) {
    resultsList_->clear();
    detailsBrowser_->clear();
    versionsTable_->setRowCount(0);
    installVersionBtn_->setEnabled(false);
    
    currentResults_ = result.hits;
    
    if (currentResults_.isEmpty()) {
        resultsList_->addItem("No results found.");
        return;
    }
    
    for (int i = 0; i < currentResults_.size(); ++i) {
        const auto& p = currentResults_[i];
        QString title = p.title;
        if (title.isEmpty()) title = p.slug;
        
        QString text = QString("%1\n%2 downloads")
            .arg(title, QString::number(p.downloads));
            
        auto* item = new QListWidgetItem(text, resultsList_);
        item->setData(Qt::UserRole, i); 
        
        if (!p.iconUrl.isEmpty()) {
            loadIcon(i, p.iconUrl);
        }
    }
}

void ModrinthBrowserDialog::loadIcon(int index, const QString& url) {
    QNetworkRequest req(url);
    req.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
    auto* reply = imgNetMgr_->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, index]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            QPixmap pix;
            if (pix.loadFromData(reply->readAll())) {
                // Scale down aggressively to reduce RAM footprint per item
                pix = pix.scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                if (index < resultsList_->count()) {
                    auto* item = resultsList_->item(index);
                    if (item && item->data(Qt::UserRole).toInt() == index) {
                        item->setIcon(QIcon(pix));
                    }
                }
            }
        }
    });
}

void ModrinthBrowserDialog::onSearchFailed(const QString& err) {
    resultsList_->clear();
    resultsList_->addItem("Search failed: " + err);
}

void ModrinthBrowserDialog::onProjectClicked(QListWidgetItem* item) {
    if (!item) return;
    int idx = item->data(Qt::UserRole).toInt();
    if (idx < 0 || idx >= currentResults_.size()) return;
    
    const auto& p = currentResults_[idx];
    currentSelectedProjectId_ = p.id;
    
    QString html = QString("<h2>%1</h2><p><b>By %2</b></p><hr><p>%3</p>")
                   .arg(p.title.isEmpty() ? p.slug : p.title, p.author, p.description);
    detailsBrowser_->setHtml(html);
    versionsTable_->setRowCount(0);
    installVersionBtn_->setEnabled(false);
    
    
    
    
    
    
    QString loaderStr;
    if (typeCombo_->currentData().toString() == "mod" && currentInstance_.loader != ModLoaderType::Vanilla) {
        loaderStr = modLoaderName(currentInstance_.loader).toLower();
    }
    
    api_->getProjectVersions(p.id, currentInstance_.mcVersion, loaderStr);
}

void ModrinthBrowserDialog::onVersionsFinished(const QString& projectId, const QVector<x4::ModrinthVersion>& versions) {
    if (projectId != currentSelectedProjectId_) return;
    
    currentVersions_ = versions;
    versionsTable_->setRowCount(versions.size());
    for (int i = 0; i < versions.size(); ++i) {
        const auto& v = versions[i];
        
        auto* nameItem = new QTableWidgetItem(v.name);
        nameItem->setFlags(nameItem->flags() ^ Qt::ItemIsEditable);
        
        auto* gameVersItem = new QTableWidgetItem(v.gameVersions.join(", "));
        gameVersItem->setFlags(gameVersItem->flags() ^ Qt::ItemIsEditable);
        
        auto* loadersItem = new QTableWidgetItem(v.loaders.join(", "));
        loadersItem->setFlags(loadersItem->flags() ^ Qt::ItemIsEditable);
        
        versionsTable_->setItem(i, 0, nameItem);
        versionsTable_->setItem(i, 1, gameVersItem);
        versionsTable_->setItem(i, 2, loadersItem);
    }
    
    // Auto-select the first (latest) version if available
    if (!versions.isEmpty()) {
        versionsTable_->selectRow(0);
        installVersionBtn_->setEnabled(true);
    }
}

void ModrinthBrowserDialog::onVersionsFailed(const QString& projectId, const QString& err) {
    if (projectId != currentSelectedProjectId_) return;
    versionsTable_->setRowCount(1);
    versionsTable_->setItem(0, 0, new QTableWidgetItem("Failed to load versions: " + err));
}



void ModrinthBrowserDialog::addToQueue(const x4::ModrinthVersion& version, const x4::ModrinthFile& file) {
    QString pt = typeCombo_->currentData().toString();
    QString subDir;
    if (pt == "mod") {
        subDir = "mods";
    } else if (pt == "resourcepack") {
        subDir = "resourcepacks";
    } else if (pt == "shader") {
        subDir = "shaderpacks";
    } else if (pt == "datapack") {
        subDir = "saves"; 
    }
    
    QString targetDir = instanceMgr_->minecraftDir(instanceId_) + "/" + subDir;
    QString targetFile = targetDir + "/" + file.filename;
    
    QueuedDownload qd;
    qd.versionId = version.id;
    qd.filename = file.filename;
    qd.url = file.url;
    qd.targetFile = targetFile;
    qd.sha1 = file.sha1;
    qd.size = file.size;
    
    
    for (const auto& item : downloadQueue_) {
        if (item.url == file.url) return;
    }
    
    downloadQueue_.append(qd);
    updateQueueUI();
}

void ModrinthBrowserDialog::updateQueueUI() {
    queueList_->clear();
    for (const auto& qd : downloadQueue_) {
        queueList_->addItem(qd.filename);
    }
    downloadQueueBtn_->setEnabled(!downloadQueue_.isEmpty());
}

void ModrinthBrowserDialog::flushQueue() {
    if (downloadQueue_.isEmpty()) return;

    QStringList versionIds;
    
    
    
    
    for (const auto& qd : downloadQueue_) {
        if (!qd.versionId.isEmpty() && !versionIds.contains(qd.versionId)) {
            versionIds.append(qd.versionId);
        }
    }

    GlobalEvents::instance().downloadStarted("Resolving dependencies...");
    
    ModLoaderType loaderToUse = ModLoaderType::Vanilla;
    if (typeCombo_->currentData().toString() == "mod") {
        loaderToUse = currentInstance_.loader;
    }
    
    api_->resolveDependencies(versionIds, currentInstance_.mcVersion, loaderToUse);
    connect(api_, &ModrinthAPI::dependenciesResolved, this, [this](const QVector<x4::ModrinthVersion>& deps) {
        
        for (const auto& v : deps) {
            auto file = v.files.first();
            for (const auto& f : v.files) if (f.primary) { file = f; break; }
            
            QString subDir = (typeCombo_->currentData().toString() == "mod") ? "mods" : "resourcepacks";
            QString targetFile = instanceMgr_->minecraftDir(instanceId_) + "/" + subDir + "/" + file.filename;
            
            
            bool dup = false;
            for (const auto& existing : downloadQueue_) {
                if (existing.sha1 == file.sha1 || existing.filename == file.filename) dup = true;
            }
            if (!dup && !file.url.isEmpty()) {
                QueuedDownload qd;
                qd.filename = file.filename;
                qd.url = file.url;
                qd.targetFile = targetFile;
                qd.sha1 = file.sha1;
                qd.size = file.size;
                downloadQueue_.append(qd);
            }
        }

        GlobalEvents::instance().downloadStarted("Downloading Modrinth queue...");

        auto* dm = new DownloadManager(this);
        QStringList targets;

        for (const auto& qd : downloadQueue_) {
            QDir().mkpath(QFileInfo(qd.targetFile).absolutePath());
            dm->enqueue(qd.url, qd.targetFile, qd.sha1, qd.size);
            targets.append(qd.targetFile);
        }
        
        connect(dm, &DownloadManager::taskProgress, this, [](const QString&, qint64 recv, qint64 total) {
            if (total > 0) {
                QString speedStr = QString("%1 / %2 MB").arg(recv / 1024.0 / 1024.0, 0, 'f', 1).arg(total / 1024.0 / 1024.0, 0, 'f', 1);
                GlobalEvents::instance().downloadProgress(static_cast<int>((recv * 100) / total), speedStr);
            }
        });
        
        connect(dm, &DownloadManager::allFinished, this, [this, dm, targets](bool success, const QStringList& errs) {
            dm->deleteLater();
            
            if (success) {
                downloadQueue_.clear();
                updateQueueUI();
                GlobalEvents::instance().downloadFinished(true, "Installed items successfully!");
            } else {
                for (const auto& path : targets) QFile::remove(path);
                GlobalEvents::instance().downloadFinished(false, "Download failed: " + errs.join(", "));
                QMessageBox::warning(this, "Error", "Download failed:\n" + errs.join("\n"));
            }
        });

        dm->startAll();
    });
}

} 
