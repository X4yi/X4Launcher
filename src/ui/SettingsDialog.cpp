#include "SettingsDialog.h"
#include "Theme.h"
#include "JavaInstallerDialog.h"
#include "../core/SettingsManager.h"
#include "../core/Translator.h"
#include "../java/JavaManager.h"
#include <QApplication>
#include <QMessageBox>
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
    SettingsDialog::SettingsDialog(JavaManager *javaMgr, QWidget *parent)
        : QDialog(parent), javaMgr_(javaMgr) {
        setWindowFlag(Qt::Window, true);
        setWindowTitle(X4TR("settings.title"));
        setMinimumSize(560, 440);
        resize(640, 500);
        setupUI();
        loadSettings();
    }

    void SettingsDialog::setupUI() {
        auto *layout = new QVBoxLayout(this);

        auto *tabs = new QTabWidget(this);
        tabs->addTab(createGeneralTab(), X4TR("settings.tab.general"));
        tabs->addTab(createJavaTab(), X4TR("settings.tab.java"));
        tabs->addTab(createMinecraftTab(), X4TR("settings.tab.mc"));
        tabs->addTab(createNetworkTab(), X4TR("settings.tab.net"));
        layout->addWidget(tabs);

        auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                             this);
        auto *saveBtn = buttons->button(QDialogButtonBox::Save);
        if (saveBtn) {
            saveBtn->setText(X4TR("btn.save"));
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

        auto *saveShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_S), this);
        connect(saveShortcut, &QShortcut::activated, this, [this]() {
            saveSettings();
            SettingsManager::instance().saveNow();
        });
    }

    QWidget *SettingsDialog::createGeneralTab() {
        auto *widget = new QWidget();
        auto *layout = new QFormLayout(widget);
        layout->setContentsMargins(16, 16, 16, 16);
        layout->setSpacing(10);

        themeCombo_ = new QComboBox(widget);
        themeCombo_->addItem(X4TR("settings.gen.theme.dark"), QStringLiteral("dark"));
        themeCombo_->addItem(X4TR("settings.gen.theme.light"), QStringLiteral("light"));
        layout->addRow(X4TR("settings.gen.theme"), themeCombo_);

        zenModeCheck_ = new QCheckBox(X4TR("settings.gen.zen"), widget);
        layout->addRow(QString(), zenModeCheck_);

        languageCombo_ = new QComboBox(widget);
        for (const QString &lang: Translator::instance().availableLanguages()) {
            languageCombo_->addItem(Translator::instance().languageDisplayName(lang), lang);
        }
        layout->addRow(X4TR("settings.gen.lang"), languageCombo_);


        closeOnLaunchCombo_ = new QComboBox(widget);
        closeOnLaunchCombo_->addItem(X4TR("settings.gen.close.keep"), 0);
        closeOnLaunchCombo_->addItem(X4TR("settings.gen.close.min"), 1);
        closeOnLaunchCombo_->addItem(X4TR("settings.gen.close.close"), 2);
        layout->addRow(X4TR("settings.gen.close"), closeOnLaunchCombo_);

        logLevelCombo_ = new QComboBox(widget);
        logLevelCombo_->addItem(X4TR("settings.gen.log.info"), 0);
        logLevelCombo_->addItem(X4TR("settings.gen.log.debug"), 1);
        layout->addRow(X4TR("settings.gen.log"), logLevelCombo_);

        return widget;
    }

    QWidget *SettingsDialog::createJavaTab() {
        auto *widget = new QWidget();
        auto *layout = new QVBoxLayout(widget);
        layout->setContentsMargins(16, 16, 16, 16);
        layout->setSpacing(10);

        auto *form = new QFormLayout();

        defaultJavaEdit_ = new QLineEdit(widget);
        defaultJavaEdit_->setPlaceholderText(X4TR("settings.java.default.ph"));
        form->addRow(X4TR("settings.java.default"), defaultJavaEdit_);

        autoDetectJava_ = new QCheckBox(X4TR("settings.java.auto"), widget);
        form->addRow(QString(), autoDetectJava_);

        layout->addLayout(form);


        auto *group = new QGroupBox(X4TR("settings.java.detected"), widget);
        auto *groupLayout = new QVBoxLayout(group);

        javaList_ = new QListWidget(widget);
        groupLayout->addWidget(javaList_);

        auto *btnLayout = new QHBoxLayout();
        auto *refreshBtn = new QPushButton(X4TR("btn.refresh"), widget);
        connect(refreshBtn, &QPushButton::clicked, this, [this]() {
            javaList_->clear();
            javaList_->addItem(X4TR("settings.java.scanning"));
            javaMgr_->refresh();
        });
        btnLayout->addWidget(refreshBtn);

        auto *downloadBtn = new QPushButton(X4TR("settings.java.dl"), widget);
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
            for (const auto &j: javaMgr_->detectedJavas()) {
                QString text = QStringLiteral("Java %1 (%2) — %3%4")
                        .arg(j.major).arg(j.arch, j.path,
                                          j.isManaged
                                              ? QStringLiteral(" [") + X4TR("settings.java.managed") + QStringLiteral(
                                                    "]")
                                              : QString());
                auto *item = new QListWidgetItem(text, javaList_);
                item->setData(Qt::UserRole, j.path);
            }
        };

        connect(javaMgr_, &JavaManager::detectionFinished, this, populateList);


        populateList();


        connect(javaList_, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {
            if (item) {
                defaultJavaEdit_->setText(item->data(Qt::UserRole).toString());
            }
        });

        return widget;
    }

    QWidget *SettingsDialog::createMinecraftTab() {
        auto *widget = new QWidget();
        auto *layout = new QFormLayout(widget);
        layout->setContentsMargins(16, 16, 16, 16);
        layout->setSpacing(10);

        auto *memGroup = new QGroupBox(X4TR("settings.mc.mem"), widget);
        auto *memLayout = new QHBoxLayout(memGroup);

        minMemSpin_ = new QSpinBox(widget);
        minMemSpin_->setRange(256, 32768);
        minMemSpin_->setSuffix(QStringLiteral(" MB"));
        memLayout->addWidget(new QLabel(X4TR("settings.mc.min")));
        memLayout->addWidget(minMemSpin_);

        maxMemSpin_ = new QSpinBox(widget);
        maxMemSpin_->setRange(512, 65536);
        maxMemSpin_->setSuffix(QStringLiteral(" MB"));
        memLayout->addWidget(new QLabel(X4TR("settings.mc.max")));
        memLayout->addWidget(maxMemSpin_);
        layout->addRow(memGroup);

        jvmArgsEdit_ = new QLineEdit(widget);
        jvmArgsEdit_->setPlaceholderText(X4TR("settings.mc.jvm.ph"));
        layout->addRow(X4TR("settings.mc.jvm"), jvmArgsEdit_);

        gameArgsEdit_ = new QLineEdit(widget);
        gameArgsEdit_->setPlaceholderText(X4TR("settings.mc.game.ph"));
        layout->addRow(X4TR("settings.mc.game"), gameArgsEdit_);

        auto *winGroup = new QGroupBox(X4TR("settings.mc.win"), widget);
        auto *winLayout = new QHBoxLayout(winGroup);

        winWidthSpin_ = new QSpinBox(widget);
        winWidthSpin_->setRange(320, 7680);
        winLayout->addWidget(new QLabel(X4TR("settings.mc.width")));
        winLayout->addWidget(winWidthSpin_);

        winHeightSpin_ = new QSpinBox(widget);
        winHeightSpin_->setRange(240, 4320);
        winLayout->addWidget(new QLabel(X4TR("settings.mc.height")));
        winLayout->addWidget(winHeightSpin_);

        fullscreenCheck_ = new QCheckBox(X4TR("settings.mc.fs"), widget);
        winLayout->addWidget(fullscreenCheck_);

        layout->addRow(winGroup);

        showSnapshots_ = new QCheckBox(X4TR("settings.mc.snap"), widget);
        layout->addRow(QString(), showSnapshots_);

        showOldAlphaCheck_ = new QCheckBox(X4TR("settings.mc.alpha"), widget);
        layout->addRow(QString(), showOldAlphaCheck_);

        return widget;
    }

    QWidget *SettingsDialog::createNetworkTab() {
        auto *widget = new QWidget();
        auto *layout = new QFormLayout(widget);
        layout->setContentsMargins(16, 16, 16, 16);
        layout->setSpacing(10);

        parallelismSpin_ = new QSpinBox(widget);
        parallelismSpin_->setRange(1, 20);
        layout->addRow(X4TR("settings.net.parallel"), parallelismSpin_);

        return widget;
    }

    void SettingsDialog::loadSettings() {
        auto &s = SettingsManager::instance();

        themeCombo_->setCurrentIndex(s.theme() == QStringLiteral("light") ? 1 : 0);
        zenModeCheck_->setChecked(s.zenMode());
        languageCombo_->setCurrentIndex(languageCombo_->findData(s.language()));
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
        auto &s = SettingsManager::instance();

        QString newTheme = themeCombo_->currentData().toString();
        if (newTheme != s.theme()) {
            s.setTheme(newTheme);
        }

        bool newZen = zenModeCheck_->isChecked();
        if (newZen != s.zenMode()) {
            s.setZenMode(newZen);
            s.saveNow();
            QMessageBox::information(this, X4TR("settings.zen.restart.title"), X4TR("settings.zen.restart.msg"));
            qApp->quit();
        }

        QString newLang = languageCombo_->currentData().toString();
        if (newLang != s.language()) {
            s.setLanguage(newLang);
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