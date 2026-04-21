#pragma once
 
#include <QDialog>
#include <QString>
#include <QVector>
#include <QTimer>
#include "../instance/Instance.h"
#include "../modrinth/ModrinthAPI.h"
 
class QLineEdit;
class QComboBox;
class QListWidget;
class QListWidgetItem;
class QTableWidget;
class QTextBrowser;
class QPushButton;
class QProgressDialog;
class QSplitter;
class QNetworkAccessManager;
class QNetworkReply;
 
namespace x4 {
 
class InstanceManager;
 
class ModrinthBrowserDialog : public QDialog {
    Q_OBJECT
public:
    explicit ModrinthBrowserDialog(const QString& instanceId, InstanceManager* instanceMgr,
                                   const QString& initialFilter = QString(), QWidget* parent = nullptr);

    /// Set the project type filter ("mod", "resourcepack", "shader", "datapack")
    void setProjectType(const QString& type);
 
private slots:
    void performSearch();
    void onSearchFinished(const x4::ModrinthSearchResult& result);
    void onSearchFailed(const QString& err);
    void onSearchTextChanged();
    
    void onProjectClicked(QListWidgetItem* item);
    
    void onVersionsFinished(const QString& projectId, const QVector<x4::ModrinthVersion>& versions);
    void onVersionsFailed(const QString& projectId, const QString& err);
 
private:
    void setupUI();
    void loadIcon(int index, const QString& url);
    void addToQueue(const x4::ModrinthVersion& version, const x4::ModrinthFile& file);
    void updateQueueUI();
    void flushQueue();
    QString instanceId_;
    InstanceManager* instanceMgr_;
     
    Instance currentInstance_;
    ModrinthAPI* api_;
     
    QLineEdit* searchEdit_;
    QComboBox* typeCombo_;
    QComboBox* sortCombo_;
    QListWidget* resultsList_;
    QPushButton* searchBtn_;
     
    QSplitter* splitter_;
    QTextBrowser* detailsBrowser_;
    QTableWidget* versionsTable_;
    QPushButton* installVersionBtn_;
 
    QListWidget* queueList_;
    QPushButton* downloadQueueBtn_;
    QPushButton* clearQueueBtn_;
 
    QNetworkAccessManager* imgNetMgr_;
    QTimer* searchTimer_;
     
    QVector<ModrinthProject> currentResults_;
    QVector<ModrinthVersion> currentVersions_;
    QString currentSelectedProjectId_;
    QString m_pendingDoubleClickProjectId;
     
    struct QueuedDownload {
        QString versionId;
        QString filename;
        QString url;
        QString targetFile;
        QString sha1;
        qint64 size;
    };
    QVector<QueuedDownload> downloadQueue_;
};
 
} 
