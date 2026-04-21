#pragma once

#include "../core/Types.h"
#include <QDialog>

class QLineEdit;
class QComboBox;
class QCheckBox;
class QLabel;

namespace x4 {

class VersionManifest;
class MetaAPI;

class CreateInstanceDialog : public QDialog {
    Q_OBJECT
public:
    CreateInstanceDialog(VersionManifest* manifest, QWidget* parent = nullptr);

    QString instanceName() const;
    QString mcVersion() const;
    ModLoaderType loaderType() const;
    QString loaderVersion() const;
    QString iconKey() const;

private:
    void setupUI();
    void populateVersions();
    void fetchLoaderVersions();

    VersionManifest* manifest_;
    MetaAPI* metaApi_;

    QLineEdit* nameEdit_;
    QComboBox* versionCombo_;
    QComboBox* iconCombo_;
    QComboBox* loaderCombo_;
    QComboBox* loaderVersionCombo_;
    QCheckBox* showSnapshots_;
    QLabel* loaderStatusLabel_;
    QLabel* manifestStatusLabel_;
};

} 
