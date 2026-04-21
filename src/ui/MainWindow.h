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
    MainWindow(InstanceManager* instanceMgr, CacheManager* cacheMgr,
               JavaManager* javaMgr, VersionManifest* manifest,
               LaunchEngine* launchEngine,
               QWidget* parent = nullptr);

private:
    void setupUI();
    void setupSidebar();
    void setupStatusBar();
    void setupKeyBinds();
    void connectSignals();

    
    void addInstance();
    void openAccounts();
    void openSettings();
    void openInstanceFolder();

    void launchInstance(const QString& id);
    void killInstance(const QString& id);
    void editInstance(const QString& id);
    void deleteInstance(const QString& id);
    void cloneInstance(const QString& id);

    void updateStatusBar();

    
    InstanceManager* instanceMgr_;
    CacheManager* cacheMgr_;
    JavaManager* javaMgr_;
    VersionManifest* manifest_;
    LaunchEngine* launchEngine_;

    
    InstanceListWidget* instanceList_;
    QComboBox* groupFilterCombo_;
    KeyBindManager* keyBinds_;
    QMap<QString, InstanceEditorDialog*> openEditors_;
    QPointer<QDialog> openCreateInstanceDialog_;
    
    QProgressBar* globalProgress_;
    QLabel* globalProgressLabel_;
    QLabel* playtimeLabel_;
    QLabel* sidebarInstanceIcon_;
    QLabel* sidebarInstanceName_;
    QLabel* premiumAvatar_;
    QLabel* accountNameLabel_;
    QNetworkAccessManager* avatarNetwork_;
    QPushButton* killButton_;
    QPointer<QDialog> openAuthDialog_;
    QPointer<QDialog> openSettingsDialog_;
};

} 
