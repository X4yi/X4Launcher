#include "JavaInstallerDialog.h"
#include "../java/JavaManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QProgressBar>
#include <QLabel>
#include <QMessageBox>

namespace x4 {
    JavaInstallerDialog::JavaInstallerDialog(JavaManager *javaMgr, QWidget *parent)
        : QDialog(parent), javaMgr_(javaMgr) {
        setWindowFlag(Qt::Window, true);
        setWindowTitle(QStringLiteral("Java Installer"));
        setMinimumSize(500, 350);

        options_ = {
            {8, QStringLiteral("Minecraft 1.16.5 and below")},
            {16, QStringLiteral("Minecraft 1.17.x")},
            {17, QStringLiteral("Minecraft 1.18 - 1.20.4")},
            {21, QStringLiteral("Minecraft 1.20.5 and above")}
        };

        setupUI();
        populateTable();

        connect(javaMgr_, &JavaManager::provisionProgress, this, [this](int percent, const QString &status) {
            progressBar_->setValue(percent);
            statusLabel_->setText(status);
        });

        connect(javaMgr_, &JavaManager::provisionFinished, this, [this](bool success, const QString &error) {
            installBtn_->setEnabled(true);
            if (success) {
                statusLabel_->setText(QStringLiteral("Installation successful!"));
                progressBar_->setValue(100);
                QMessageBox::information(this, QStringLiteral("Success"),
                                         QStringLiteral("Java installed successfully."));
                accept();
            } else {
                statusLabel_->setText(QStringLiteral("Installation failed."));
                progressBar_->setValue(0);
                QMessageBox::critical(this, QStringLiteral("Error"), error);
            }
        });
    }

    void JavaInstallerDialog::setupUI() {
        auto *layout = new QVBoxLayout(this);

        auto *label = new QLabel(QStringLiteral("Select a Java version to automatically download and install:"), this);
        layout->addWidget(label);

        table_ = new QTableWidget(this);
        table_->setColumnCount(2);
        table_->setHorizontalHeaderLabels({QStringLiteral("Java Version"), QStringLiteral("Recommended For")});
        table_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
        table_->setSelectionBehavior(QAbstractItemView::SelectRows);
        table_->setSelectionMode(QAbstractItemView::SingleSelection);
        table_->verticalHeader()->setVisible(false);
        layout->addWidget(table_);

        statusLabel_ = new QLabel(this);
        layout->addWidget(statusLabel_);

        progressBar_ = new QProgressBar(this);
        progressBar_->setRange(0, 100);
        progressBar_->setValue(0);
        layout->addWidget(progressBar_);

        auto *btnLayout = new QHBoxLayout();
        btnLayout->addStretch();

        installBtn_ = new QPushButton(QStringLiteral("Install Selected"), this);
        installBtn_->setEnabled(false);
        btnLayout->addWidget(installBtn_);

        closeBtn_ = new QPushButton(QStringLiteral("Cancel"), this);
        btnLayout->addWidget(closeBtn_);

        layout->addLayout(btnLayout);

        connect(table_, &QTableWidget::itemSelectionChanged, this, &JavaInstallerDialog::onSelectionChanged);
        connect(installBtn_, &QPushButton::clicked, this, &JavaInstallerDialog::onInstallClicked);
        connect(closeBtn_, &QPushButton::clicked, this, &QDialog::reject);
    }

    void JavaInstallerDialog::populateTable() {
        table_->setRowCount(options_.size());
        for (int i = 0; i < options_.size(); ++i) {
            auto *verItem = new QTableWidgetItem(QStringLiteral("Java %1").arg(options_[i].major));
            verItem->setData(Qt::UserRole, options_[i].major);
            verItem->setFlags(verItem->flags() ^ Qt::ItemIsEditable);

            auto *descItem = new QTableWidgetItem(options_[i].description);
            descItem->setFlags(descItem->flags() ^ Qt::ItemIsEditable);

            table_->setItem(i, 0, verItem);
            table_->setItem(i, 1, descItem);
        }
    }

    void JavaInstallerDialog::onSelectionChanged() {
        installBtn_->setEnabled(!table_->selectedItems().isEmpty());
    }

    void JavaInstallerDialog::onInstallClicked() {
        auto items = table_->selectedItems();
        if (items.isEmpty()) return;

        int major = items.first()->data(Qt::UserRole).toInt();

        installBtn_->setEnabled(false);
        table_->setEnabled(false);
        progressBar_->setValue(0);
        statusLabel_->setText(QStringLiteral("Starting download..."));

        javaMgr_->provisionJava(major);
    }
}