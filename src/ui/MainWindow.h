#pragma once

#include <QMainWindow>
#include <QMap>
#include <QProgressBar>
#include <QLabel>
#include <QComboBox>

class QNetworkAccessManager;
class QPushButton;

#include <QPointer>

namespace x4 {
    class InstanceManager;
    class CacheManager;
    class JavaManager;
    class VersionManifest;
    class LaunchEngine;
    class InstanceListWidget;
    class InstanceEditorDialog;
    class KeyBindManager;


    class MainWindow : public QMainWindow {
        Q_OBJECT

    public:
        MainWindow(InstanceManager *instanceMgr, CacheManager *cacheMgr,
                   JavaManager *javaMgr, VersionManifest *manifest,
                   LaunchEngine *launchEngine,
                   QWidget *parent = nullptr);

        void applyZenMode(bool zen);

    private:
        void setupUI();

        void setupZenUI();

        void setupSidebar();

        void setupStatusBar();

        void setupKeyBinds();

        void connectSignals();

        void updateSelectedInstanceInfo();


        void addInstance();

        void openAccounts();

        void openSettings();

        void openInstanceFolder();

        void launchInstance(const QString &id);

        void killInstance(const QString &id);

        void editInstance(const QString &id);

        void deleteInstance(const QString &id);

        void cloneInstance(const QString &id);

        void updateStatusBar();


        InstanceManager *instanceMgr_;
        CacheManager *cacheMgr_;
        JavaManager *javaMgr_;
        VersionManifest *manifest_;
        LaunchEngine *launchEngine_;


        InstanceListWidget *instanceList_;
        QComboBox *groupFilterCombo_;
        KeyBindManager *keyBinds_;
        QMap<QString, InstanceEditorDialog *> openEditors_;
        QPointer<QDialog> openCreateInstanceDialog_;

        QProgressBar *globalProgress_ = nullptr;
        QLabel *globalProgressLabel_ = nullptr;
        QLabel *playtimeLabel_ = nullptr;
        QLabel *sidebarInstanceIcon_ = nullptr;
        QLabel *sidebarInstanceName_ = nullptr;
        QLabel *premiumAvatar_ = nullptr;
        QLabel *accountNameLabel_ = nullptr;
        QNetworkAccessManager *avatarNetwork_ = nullptr;
        QPushButton *killButton_ = nullptr;
        QPointer<QDialog> openAuthDialog_;
        QPointer<QDialog> openSettingsDialog_;
        bool zenMode_ = false;
    };
}