#pragma once

#include "../core/Types.h"
#include <QDialog>
#include <QString>

class QLineEdit;
class QComboBox;
class QCheckBox;
class QLabel;
class QButtonGroup;

namespace x4 {
    class VersionManifest;
    class MetaAPI;

    class CreateInstanceDialog : public QDialog {
        Q_OBJECT

    public:
        CreateInstanceDialog(VersionManifest *manifest, QWidget *parent = nullptr);

        QString instanceName() const;

        QString mcVersion() const;

        ModLoaderType loaderType() const;

        QString loaderVersion() const;

        QString iconKey() const;

    private:
        void setupUI();

        void populateVersions();

        void fetchLoaderVersions();

        void updateSummary();

        VersionManifest *manifest_;
        MetaAPI *metaApi_;

        QLineEdit *nameEdit_;
        QComboBox *versionCombo_;
        QButtonGroup *iconGroup_;
        QComboBox *loaderCombo_;
        QComboBox *loaderVersionCombo_;
        QCheckBox *showSnapshots_;
        QLabel *loaderStatusLabel_;
        QLabel *manifestStatusLabel_;
        QLabel *summaryLabel_;

        QString selectedIconKey_ = QStringLiteral("vanilla_icon");
    };
} // namespace x4