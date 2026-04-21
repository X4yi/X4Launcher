#pragma once

#include <QDialog>
#include <QSet>
class QTabWidget;
class QListWidget;
class QLineEdit;
class QComboBox;
class QSpinBox;
class QPlainTextEdit;
class QCheckBox;
class QLabel;

namespace x4 {

class InstanceManager;
class JavaManager;
class VersionManifest;
class MetaAPI;
class ConsoleWidget;
struct Instance;
class InstanceConfig;
class DownloadManager;
class ModrinthBrowserDialog;
class ModrinthAPI;

class InstanceEditorDialog : public QDialog {
    Q_OBJECT
public:
    InstanceEditorDialog(InstanceManager* mgr, JavaManager* javaMgr,
                         VersionManifest* manifest, const QString& instanceId,
                         QWidget* parent = nullptr);

    ConsoleWidget* consoleWidget() { return console_; }

    void switchToConsoleTab();

signals:
    void instanceModified(const QString& instanceId);

private:
    void setupUI();
    QWidget* createOverviewTab();
    QWidget* createModsTab();
    QWidget* createWorldsTab();
    QWidget* createResourcePacksTab();
    QWidget* createShaderPacksTab();
    QWidget* createSettingsTab();
    QWidget* createConsoleTab();

    void loadData();
    void saveData();
    void refreshModList();
    void refreshWorldList();
    void refreshResourcePackList();
    void refreshShaderPackList();
    void populateFileList(QListWidget* list, const QString& dir, const QStringList& filters);
    void fetchLoaderVersions();

    InstanceManager* mgr_;
    JavaManager* javaMgr_;
    VersionManifest* manifest_;
    MetaAPI* metaApi_;
    QString instanceId_;

    
    QLineEdit* nameEdit_;
    QComboBox* iconCombo_;
    QComboBox* versionCombo_;
    QComboBox* loaderCombo_;
    QComboBox* loaderVersionCombo_;
    QLabel* loaderStatusLabel_;
     QCheckBox* showSnapshots_;

    
    QLineEdit* javaPathEdit_;
    QListWidget* javaList_;
    QSpinBox* minMemSpin_;
    QSpinBox* maxMemSpin_;
    QLineEdit* jvmArgsEdit_;
    QLineEdit* gameArgsEdit_;
    QSpinBox* winWidthSpin_;
    QSpinBox* winHeightSpin_;
    QCheckBox* fullscreenCheck_;

    
    QListWidget* modList_;
    QListWidget* worldList_;
    QListWidget* resourcePackList_;
    QListWidget* shaderPackList_;

    
    ModrinthBrowserDialog* modrinthBrowser_ = nullptr;
    ModrinthAPI* modrinthApi_;

    
    ConsoleWidget* console_;

    QTabWidget* tabs_;
    QSet<int> loadedTabs_;

    
    void populateRichFileList(QListWidget* list, const QString& dir, const QStringList& filters);
};

} 
