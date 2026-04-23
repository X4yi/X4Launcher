#pragma once

#include <QDialog>

class QLineEdit;
class QComboBox;
class QSpinBox;
class QListWidget;
class QCheckBox;

namespace x4 {

class JavaManager;


class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    SettingsDialog(JavaManager* javaMgr, QWidget* parent = nullptr);

private:
    void setupUI();
    QWidget* createGeneralTab();
    QWidget* createJavaTab();
    QWidget* createMinecraftTab();
    QWidget* createNetworkTab();

    void loadSettings();
    void saveSettings();

    JavaManager* javaMgr_;

    
    QComboBox* themeCombo_;
    QCheckBox* zenModeCheck_;
    QComboBox* languageCombo_;
    QComboBox* closeOnLaunchCombo_;
    QComboBox* logLevelCombo_;

    
    QLineEdit* defaultJavaEdit_;
    QCheckBox* autoDetectJava_;
    QListWidget* javaList_;

    
    QSpinBox* minMemSpin_;
    QSpinBox* maxMemSpin_;
    QLineEdit* jvmArgsEdit_;
    QLineEdit* gameArgsEdit_;
    QSpinBox* winWidthSpin_;
    QSpinBox* winHeightSpin_;
    QCheckBox* fullscreenCheck_;
    QCheckBox* showSnapshots_;
    QCheckBox* showOldAlphaCheck_;

    
    QSpinBox* parallelismSpin_;
};

} 
