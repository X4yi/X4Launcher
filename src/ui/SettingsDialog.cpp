#include "SettingsDialog.h"
#include "Theme.h"
#include "JavaInstallerDialog.h"
#include "../core/SettingsManager.h"
#include "../java/JavaManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QTabWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QListWidget>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QShortcut>

namespace x4 {

SettingsDialog::SettingsDialog(JavaManager* javaMgr, QWidget* parent)
    : QDialog(parent), javaMgr_(javaMgr)
{
    setWindowTitle(QStringLiteral("Settings"));
    setMinimumSize(560, 440);
    resize(640, 500);
    setupUI();
    loadSettings();
}

void SettingsDialog::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* tabs = new QTabWidget(this);
    tabs->addTab(createGeneralTab(), QStringLiteral("General"));
    tabs->addTab(createJavaTab(), QStringLiteral("Java"));
    tabs->addTab(createMinecraftTab(), QStringLiteral("Minecraft"));
    tabs->addTab(createNetworkTab(), QStringLiteral("Network"));
    layout->addWidget(tabs);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    auto* saveBtn = buttons->button(QDialogButtonBox::Save);
    if (saveBtn) {
        saveBtn->setText(QStringLiteral("Save"));
        connect(saveBtn, &QPushButton::clicked, this, [this]() {
            saveSettings();
            SettingsManager::instance().saveNow();
        });
    }
    connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
        saveSettings();
        SettingsManager::instance().saveNow();
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);

    auto* saveShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_S), this);
    connect(saveShortcut, &QShortcut::activated, this, [this]() {
        saveSettings();
        SettingsManager::instance().saveNow();
    });
}

QWidget* SettingsDialog::createGeneralTab() {
    auto* widget = new QWidget();
    auto* layout = new QFormLayout(widget);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(10);

    themeCombo_ = new QComboBox(widget);
    themeCombo_->addItem(QStringLiteral("Dark"), QStringLiteral("dark"));
    themeCombo_->addItem(QStringLiteral("Light"), QStringLiteral("light"));
    layout->addRow(QStringLiteral("Theme:"), themeCombo_);


    closeOnLaunchCombo_ = new QComboBox(widget);
    closeOnLaunchCombo_->addItem(QStringLiteral("Keep launcher open"), 0);
    closeOnLaunchCombo_->addItem(QStringLiteral("Minimize launcher"), 1);
    closeOnLaunchCombo_->addItem(QStringLiteral("Close launcher"), 2);
    layout->addRow(QStringLiteral("On Game Launch:"), closeOnLaunchCombo_);

    logLevelCombo_ = new QComboBox(widget);
    logLevelCombo_->addItem(QStringLiteral("Info"), 0);
    logLevelCombo_->addItem(QStringLiteral("Debug"), 1);
    layout->addRow(QStringLiteral("Log Level:"), logLevelCombo_);

    return widget;
}

QWidget* SettingsDialog::createJavaTab() {
    auto* widget = new QWidget();
    auto* layout = new QVBoxLayout(widget);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(10);

    auto* form = new QFormLayout();

    defaultJavaEdit_ = new QLineEdit(widget);
    defaultJavaEdit_->setPlaceholderText(QStringLiteral("(auto-detect)"));
    form->addRow(QStringLiteral("Default Java:"), defaultJavaEdit_);

    autoDetectJava_ = new QCheckBox(QStringLiteral("Auto-detect Java for each Minecraft version"), widget);
    form->addRow(QString(), autoDetectJava_);

    layout->addLayout(form);

    
    auto* group = new QGroupBox(QStringLiteral("Detected Java Installations"), widget);
    auto* groupLayout = new QVBoxLayout(group);

    javaList_ = new QListWidget(widget);
    groupLayout->addWidget(javaList_);

    auto* btnLayout = new QHBoxLayout();
    auto* refreshBtn = new QPushButton(QStringLiteral("Refresh"), widget);
    connect(refreshBtn, &QPushButton::clicked, this, [this]() {
        javaList_->clear();
        javaList_->addItem(QStringLiteral("Scanning..."));
        javaMgr_->refresh();
    });
    btnLayout->addWidget(refreshBtn);

    auto* downloadBtn = new QPushButton(QStringLiteral("Download Java..."), widget);
    connect(downloadBtn, &QPushButton::clicked, this, [this]() {
        JavaInstallerDialog dlg(javaMgr_, this);
        dlg.exec();
    });
    btnLayout->addWidget(downloadBtn);
    btnLayout->addStretch();
    groupLayout->addLayout(btnLayout);

    layout->addWidget(group);

    
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
            defaultJavaEdit_->setText(item->data(Qt::UserRole).toString());
        }
    });

    return widget;
}

QWidget* SettingsDialog::createMinecraftTab() {
    auto* widget = new QWidget();
    auto* layout = new QFormLayout(widget);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(10);

    auto* memGroup = new QGroupBox(QStringLiteral("Default Memory"), widget);
    auto* memLayout = new QHBoxLayout(memGroup);

    minMemSpin_ = new QSpinBox(widget);
    minMemSpin_->setRange(256, 32768);
    minMemSpin_->setSuffix(QStringLiteral(" MB"));
    memLayout->addWidget(new QLabel(QStringLiteral("Min:")));
    memLayout->addWidget(minMemSpin_);

    maxMemSpin_ = new QSpinBox(widget);
    maxMemSpin_->setRange(512, 65536);
    maxMemSpin_->setSuffix(QStringLiteral(" MB"));
    memLayout->addWidget(new QLabel(QStringLiteral("Max:")));
    memLayout->addWidget(maxMemSpin_);
    layout->addRow(memGroup);

    jvmArgsEdit_ = new QLineEdit(widget);
    jvmArgsEdit_->setPlaceholderText(QStringLiteral("-XX:+UseG1GC"));
    layout->addRow(QStringLiteral("Default JVM Args:"), jvmArgsEdit_);

    gameArgsEdit_ = new QLineEdit(widget);
    gameArgsEdit_->setPlaceholderText(QStringLiteral("(e.g. --server mc.example.com)"));
    layout->addRow(QStringLiteral("Default Game Args:"), gameArgsEdit_);

    auto* winGroup = new QGroupBox(QStringLiteral("Game Window Size"), widget);
    auto* winLayout = new QHBoxLayout(winGroup);

    winWidthSpin_ = new QSpinBox(widget);
    winWidthSpin_->setRange(320, 7680);
    winLayout->addWidget(new QLabel(QStringLiteral("Width:")));
    winLayout->addWidget(winWidthSpin_);

    winHeightSpin_ = new QSpinBox(widget);
    winHeightSpin_->setRange(240, 4320);
    winLayout->addWidget(new QLabel(QStringLiteral("Height:")));
    winLayout->addWidget(winHeightSpin_);
    
    fullscreenCheck_ = new QCheckBox(QStringLiteral("Fullscreen"), widget);
    winLayout->addWidget(fullscreenCheck_);
    
    layout->addRow(winGroup);

    showSnapshots_ = new QCheckBox(QStringLiteral("Show snapshot versions"), widget);
    layout->addRow(QString(), showSnapshots_);

    showOldAlphaCheck_ = new QCheckBox(QStringLiteral("Show old alpha/beta versions"), widget);
    layout->addRow(QString(), showOldAlphaCheck_);

    return widget;
}

QWidget* SettingsDialog::createNetworkTab() {
    auto* widget = new QWidget();
    auto* layout = new QFormLayout(widget);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(10);

    parallelismSpin_ = new QSpinBox(widget);
    parallelismSpin_->setRange(1, 20);
    layout->addRow(QStringLiteral("Parallel Downloads:"), parallelismSpin_);

    return widget;
}

void SettingsDialog::loadSettings() {
    auto& s = SettingsManager::instance();

    themeCombo_->setCurrentIndex(s.theme() == QStringLiteral("light") ? 1 : 0);
    defaultJavaEdit_->setText(s.defaultJavaPath());
    autoDetectJava_->setChecked(s.autoDetectJava());
    minMemSpin_->setValue(s.minMemoryMB());
    maxMemSpin_->setValue(s.maxMemoryMB());
    jvmArgsEdit_->setText(s.defaultJvmArgs());
    gameArgsEdit_->setText(s.defaultGameArgs());
    winWidthSpin_->setValue(s.gameWindowWidth());
    winHeightSpin_->setValue(s.gameWindowHeight());
    fullscreenCheck_->setChecked(s.fullscreen());
    showSnapshots_->setChecked(s.showSnapshots());
    showOldAlphaCheck_->setChecked(s.showOldAlpha());
    parallelismSpin_->setValue(s.downloadParallelism());
    
    closeOnLaunchCombo_->setCurrentIndex(closeOnLaunchCombo_->findData(s.closeOnLaunch()));
    logLevelCombo_->setCurrentIndex(logLevelCombo_->findData(s.logLevel()));
}

void SettingsDialog::saveSettings() {
    auto& s = SettingsManager::instance();

    QString newTheme = themeCombo_->currentData().toString();
    if (newTheme != s.theme()) {
        s.setTheme(newTheme);
        Theme::apply(newTheme);
    }

    s.setDefaultJavaPath(defaultJavaEdit_->text().trimmed());
    s.setAutoDetectJava(autoDetectJava_->isChecked());
    s.setMinMemoryMB(minMemSpin_->value());
    s.setMaxMemoryMB(maxMemSpin_->value());
    s.setDefaultJvmArgs(jvmArgsEdit_->text().trimmed());
    s.setDefaultGameArgs(gameArgsEdit_->text().trimmed());
    s.setGameWindowWidth(winWidthSpin_->value());
    s.setGameWindowHeight(winHeightSpin_->value());
    s.setFullscreen(fullscreenCheck_->isChecked());
    s.setShowSnapshots(showSnapshots_->isChecked());
    s.setShowOldAlpha(showOldAlphaCheck_->isChecked());
    s.setDownloadParallelism(parallelismSpin_->value());
    s.setCloseOnLaunch(closeOnLaunchCombo_->currentData().toInt());
    s.setLogLevel(logLevelCombo_->currentData().toInt());
}

} 
