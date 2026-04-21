#include "CreateInstanceDialog.h"
#include "../launch/VersionManifest.h"
#include "../launch/MetaAPI.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QLabel>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

namespace x4 {

CreateInstanceDialog::CreateInstanceDialog(VersionManifest* manifest, QWidget* parent)
    : QDialog(parent), manifest_(manifest)
{
    setWindowTitle(QStringLiteral("Create New Instance"));
    setMinimumWidth(480);

    metaApi_ = new MetaAPI(this);

    connect(metaApi_, &MetaAPI::indexFinished, this, [this](const QString&, const QByteArray& data) {
        auto doc = QJsonDocument::fromJson(data);
        auto versions = doc.object()[QStringLiteral("versions")].toArray();

        QString selectedMC = versionCombo_->currentText();

        loaderVersionCombo_->clear();
        loaderVersionCombo_->addItem(QStringLiteral("(latest compatible)"), QString());

        for (const auto& item : versions) {
            auto vobj = item.toObject();
            auto reqs = vobj[QStringLiteral("requires")].toArray();
            bool compatible = false;

            if (reqs.isEmpty()) {
                compatible = true;
            } else {
                bool hasMCReq = false;
                for (const auto& reqItem : reqs) {
                    auto r = reqItem.toObject();
                    if (r[QStringLiteral("uid")].toString() == QStringLiteral("net.minecraft")) {
                        hasMCReq = true;
                        QString eq = r[QStringLiteral("equals")].toString();
                        if (eq == selectedMC || eq.isEmpty())
                            compatible = true;
                    }
                }
                if (!hasMCReq)
                    compatible = true;
            }

            if (compatible) {
                QString ver = vobj[QStringLiteral("version")].toString();
                bool rec = vobj[QStringLiteral("recommended")].toBool();
                QString label = rec ? ver + QStringLiteral(" ★") : ver;
                loaderVersionCombo_->addItem(label, ver);
            }
        }

        loaderVersionCombo_->setEnabled(loaderVersionCombo_->count() > 1);
        loaderStatusLabel_->setText(
            loaderVersionCombo_->count() > 1
            ? QStringLiteral("%1 version(s) found").arg(loaderVersionCombo_->count() - 1)
            : QStringLiteral("No compatible versions found"));
    });

    connect(metaApi_, &MetaAPI::indexFailed, this, [this](const QString&, const QString& err) {
        loaderStatusLabel_->setText(QStringLiteral("Failed: %1").arg(err));
        loaderVersionCombo_->clear();
        loaderVersionCombo_->setEnabled(false);
    });

    connect(manifest_, &VersionManifest::ready, this, [this]() {
        populateVersions();
        manifestStatusLabel_->setText(QStringLiteral("Version list loaded."));
    });

    connect(manifest_, &VersionManifest::fetchError, this, [this](const QString& err) {
        if (manifest_->versions().isEmpty()) {
            manifestStatusLabel_->setText(QStringLiteral("Failed to load versions: %1").arg(err));
        }
    });

    setupUI();
    populateVersions();
}

void CreateInstanceDialog::setupUI() {
    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(12);
    layout->setContentsMargins(20, 20, 20, 20);

    auto* title = new QLabel(QStringLiteral("New Instance"), this);
    title->setObjectName(QStringLiteral("titleLabel"));
    layout->addWidget(title);

    auto* form = new QFormLayout();
    form->setSpacing(10);

    nameEdit_ = new QLineEdit(this);
    nameEdit_->setPlaceholderText(QStringLiteral("My Instance"));
    form->addRow(QStringLiteral("Name:"), nameEdit_);

    versionCombo_ = new QComboBox(this);
    versionCombo_->setMaxVisibleItems(15);
    versionCombo_->setEditable(true);
    versionCombo_->setInsertPolicy(QComboBox::NoInsert);
    form->addRow(QStringLiteral("Minecraft:"), versionCombo_);

    manifestStatusLabel_ = new QLabel(this);
    manifestStatusLabel_->setStyleSheet(QStringLiteral("color: gray; font-size: 11px;"));
    form->addRow(QString(), manifestStatusLabel_);

    iconCombo_ = new QComboBox(this);
    iconCombo_->setIconSize(QSize(24, 24));
    iconCombo_->addItem(QIcon(QStringLiteral(":/icons/vanilla_icon.png")), QStringLiteral("Vanilla"), QStringLiteral("vanilla_icon"));
    iconCombo_->addItem(QIcon(QStringLiteral(":/icons/steve_icon.png")), QStringLiteral("Steve"), QStringLiteral("steve_icon"));
    iconCombo_->addItem(QIcon(QStringLiteral(":/icons/forge_icon.png")), QStringLiteral("Forge"), QStringLiteral("forge_icon"));
    iconCombo_->addItem(QIcon(QStringLiteral(":/icons/fabric_icon.png")), QStringLiteral("Fabric"), QStringLiteral("fabric_icon"));
    iconCombo_->addItem(QIcon(QStringLiteral(":/icons/creeper_icon.png")), QStringLiteral("Creeper"), QStringLiteral("creeper_icon"));
    iconCombo_->addItem(QIcon(QStringLiteral(":/icons/chicken_icon.png")), QStringLiteral("Chicken"), QStringLiteral("chicken_icon"));
    iconCombo_->addItem(QIcon(QStringLiteral(":/icons/goat_icon.png")), QStringLiteral("Goat"), QStringLiteral("goat_icon"));
    iconCombo_->addItem(QIcon(QStringLiteral(":/icons/cat_icon.png")), QStringLiteral("Cat"), QStringLiteral("cat_icon"));
    form->addRow(QStringLiteral("Icon:"), iconCombo_);

    showSnapshots_ = new QCheckBox(QStringLiteral("Show snapshots"), this);
    connect(showSnapshots_, &QCheckBox::toggled, this, &CreateInstanceDialog::populateVersions);
    form->addRow(QString(), showSnapshots_);

    loaderCombo_ = new QComboBox(this);
    loaderCombo_->addItem(QStringLiteral("Vanilla"), static_cast<int>(ModLoaderType::Vanilla));
    loaderCombo_->addItem(QStringLiteral("Fabric"), static_cast<int>(ModLoaderType::Fabric));
    loaderCombo_->addItem(QStringLiteral("Forge"), static_cast<int>(ModLoaderType::Forge));
    loaderCombo_->addItem(QStringLiteral("NeoForge"), static_cast<int>(ModLoaderType::NeoForge));
    loaderCombo_->addItem(QStringLiteral("Quilt"), static_cast<int>(ModLoaderType::Quilt));
    form->addRow(QStringLiteral("Mod Loader:"), loaderCombo_);

    loaderVersionCombo_ = new QComboBox(this);
    loaderVersionCombo_->setEnabled(false);
    loaderVersionCombo_->setMaxVisibleItems(20);
    form->addRow(QStringLiteral("Loader Version:"), loaderVersionCombo_);

    loaderStatusLabel_ = new QLabel(this);
    loaderStatusLabel_->setStyleSheet(QStringLiteral("color: gray; font-size: 11px;"));
    form->addRow(QString(), loaderStatusLabel_);

    layout->addLayout(form);

    if (!manifest_->isLoaded()) {
        manifestStatusLabel_->setText(QStringLiteral("Loading version list..."));
    } else if (versionCombo_->count() > 0) {
        manifestStatusLabel_->setText(QStringLiteral("Version list loaded."));
    } else {
        manifestStatusLabel_->setText(QStringLiteral("No versions available."));
    }

    connect(versionCombo_, &QComboBox::currentTextChanged, this, [this](const QString& ver) {
        if (nameEdit_->text().isEmpty() || nameEdit_->text().startsWith(QStringLiteral("Minecraft "))) {
            nameEdit_->setText(QStringLiteral("Minecraft %1").arg(ver));
        }
        fetchLoaderVersions();
    });

    connect(loaderCombo_, &QComboBox::currentIndexChanged, this, [this]() {
        fetchLoaderVersions();
    });

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("Create"));

    connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
        if (nameEdit_->text().trimmed().isEmpty()) {
            nameEdit_->setFocus();
            return;
        }
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    layout->addStretch();
    layout->addWidget(buttons);
}

void CreateInstanceDialog::populateVersions() {
    versionCombo_->clear();
    bool snapshots = showSnapshots_->isChecked();

    for (const auto& ver : manifest_->versions()) {
        if (ver.type == QStringLiteral("release")) {
            versionCombo_->addItem(ver.id);
        } else if (snapshots && ver.type == QStringLiteral("snapshot")) {
            versionCombo_->addItem(ver.id);
        }
    }
}

void CreateInstanceDialog::fetchLoaderVersions() {
    auto loader = static_cast<ModLoaderType>(loaderCombo_->currentData().toInt());

    loaderVersionCombo_->clear();
    loaderVersionCombo_->setEnabled(false);
    loaderStatusLabel_->clear();

    if (loader == ModLoaderType::Vanilla) {
        loaderStatusLabel_->setText(QStringLiteral("No loader needed"));
        return;
    }

    QString uid;
    switch (loader) {
        case ModLoaderType::Fabric:  uid = QStringLiteral("net.fabricmc.fabric-loader"); break;
        case ModLoaderType::Forge:   uid = QStringLiteral("net.minecraftforge"); break;
        case ModLoaderType::NeoForge: uid = QStringLiteral("net.neoforged"); break;
        case ModLoaderType::Quilt:   uid = QStringLiteral("org.quiltmc.quilt-loader"); break;
        default: return;
    }

    loaderStatusLabel_->setText(QStringLiteral("Loading versions..."));
    metaApi_->getPackageIndex(uid);
}

QString CreateInstanceDialog::instanceName() const {
    return nameEdit_->text().trimmed();
}

QString CreateInstanceDialog::mcVersion() const {
    return versionCombo_->currentText();
}

ModLoaderType CreateInstanceDialog::loaderType() const {
    return static_cast<ModLoaderType>(loaderCombo_->currentData().toInt());
}

QString CreateInstanceDialog::loaderVersion() const {
    return loaderVersionCombo_->currentData().toString();
}

QString CreateInstanceDialog::iconKey() const {
    return iconCombo_->currentData().toString();
}

} 
