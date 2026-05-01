#include "CreateInstanceDialog.h"
#include "../launch/VersionManifest.h"
#include "../launch/MetaAPI.h"
#include "../core/Translator.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
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
#include <QGroupBox>
#include <QToolButton>
#include <QButtonGroup>
#include <QGridLayout>

namespace x4 {
    CreateInstanceDialog::CreateInstanceDialog(VersionManifest *manifest, QWidget *parent)
        : QDialog(parent), manifest_(manifest) {
        setWindowFlag(Qt::Window, true);
        setWindowTitle(X4TR("create.title"));
        setMinimumSize(520, 500);

        metaApi_ = new MetaAPI(this);

        connect(metaApi_, &MetaAPI::indexFinished, this, [this](const QString &, const QByteArray &data) {
            auto doc = QJsonDocument::fromJson(data);
            auto versions = doc.object()[QStringLiteral("versions")].toArray();

            QString selectedMC = versionCombo_->currentText();

            loaderVersionCombo_->clear();
            loaderVersionCombo_->addItem(QStringLiteral("(latest compatible)"), QString());

            for (const auto &item: versions) {
                auto vobj = item.toObject();
                auto reqs = vobj[QStringLiteral("requires")].toArray();
                bool compatible = false;

                if (reqs.isEmpty()) {
                    compatible = true;
                } else {
                    bool hasMCReq = false;
                    for (const auto &reqItem: reqs) {
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
                    ? X4TR1("create.loader.found", QString::number(loaderVersionCombo_->count() - 1))
                    : X4TR("create.loader.notfound"));

            updateSummary();
        });

        connect(metaApi_, &MetaAPI::indexFailed, this, [this](const QString &, const QString &err) {
            loaderStatusLabel_->setText(X4TR1("create.loader.fail", err));
            loaderVersionCombo_->clear();
            loaderVersionCombo_->setEnabled(false);
            updateSummary();
        });

        connect(manifest_, &VersionManifest::ready, this, [this]() {
            populateVersions();
            manifestStatusLabel_->setText(X4TR("create.manifest.loaded"));
        });

        connect(manifest_, &VersionManifest::fetchError, this, [this](const QString &err) {
            if (manifest_->versions().isEmpty()) {
                manifestStatusLabel_->setText(X4TR1("create.manifest.fail", err));
            }
        });

        setupUI();
        populateVersions();
    }

    void CreateInstanceDialog::setupUI() {
        auto *layout = new QVBoxLayout(this);
        layout->setSpacing(16);
        layout->setContentsMargins(24, 24, 24, 24);

        // Header Title
        auto *title = new QLabel(X4TR("create.title"), this);
        QFont titleFont = title->font();
        titleFont.setPixelSize(22);
        titleFont.setBold(true);
        title->setFont(titleFont);
        title->setStyleSheet(QStringLiteral("color: #c0caf5; margin-bottom: 8px;"));
        layout->addWidget(title);

        // Basic Info Group
        auto *basicGroup = new QGroupBox(X4TR("create.basic"), this);
        basicGroup->setStyleSheet(QStringLiteral(
            "QGroupBox { font-weight: bold; border: 1px solid #292e42; border-radius: 6px; margin-top: 10px; } QGroupBox::title { subcontrol-origin: margin; left: 10px; color: #7aa2f7; }"));
        auto *basicLayout = new QVBoxLayout(basicGroup);
        basicLayout->setSpacing(12);

        auto *nameLayout = new QHBoxLayout();
        nameLayout->addWidget(new QLabel(X4TR("create.name"), this));
        nameEdit_ = new QLineEdit(this);
        nameEdit_->setPlaceholderText(X4TR("create.name.ph"));
        nameLayout->addWidget(nameEdit_, 1);
        basicLayout->addLayout(nameLayout);

        basicLayout->addWidget(new QLabel(X4TR("create.icon"), this));

        // Icon Strip
        auto *iconStripLayout = new QGridLayout();
        iconGroup_ = new QButtonGroup(this);
        iconGroup_->setExclusive(true);

        struct IconDef {
            QString key;
            QString label;
        };
        QVector<IconDef> icons = {
            {"vanilla_icon", "Vanilla"}, {"steve_icon", "Steve"},
            {"forge_icon", "Forge"}, {"fabric_icon", "Fabric"},
            {"creeper_icon", "Creeper"}, {"chicken_icon", "Chicken"},
            {"goat_icon", "Goat"}, {"cat_icon", "Cat"}
        };

        for (int i = 0; i < icons.size(); ++i) {
            auto *btn = new QToolButton(this);
            btn->setIcon(QIcon(QStringLiteral(":/icons/") + icons[i].key + QStringLiteral(".png")));
            btn->setIconSize(QSize(48, 48));
            btn->setCheckable(true);
            btn->setToolTip(icons[i].label);
            btn->setStyleSheet(QStringLiteral(
                "QToolButton { border: 2px solid transparent; border-radius: 8px; background: transparent; padding: 4px; }"
                "QToolButton:checked { border: 2px solid #7aa2f7; background: #1f2335; }"
                "QToolButton:hover:!checked { background: #292e42; }"));
            if (i == 0) btn->setChecked(true); // Default to vanilla

            iconGroup_->addButton(btn, i);
            iconStripLayout->addWidget(btn, i / 4, i % 4);
        }
        connect(iconGroup_, &QButtonGroup::idClicked, this, [this, icons](int id) {
            if (id >= 0 && id < icons.size()) {
                selectedIconKey_ = icons[id].key;
                updateSummary();
            }
        });

        basicLayout->addLayout(iconStripLayout);
        layout->addWidget(basicGroup);

        // Version Config Group
        auto *verGroup = new QGroupBox(X4TR("create.version_config"), this);
        verGroup->setStyleSheet(QStringLiteral(
            "QGroupBox { font-weight: bold; border: 1px solid #292e42; border-radius: 6px; margin-top: 10px; } QGroupBox::title { subcontrol-origin: margin; left: 10px; color: #7aa2f7; }"));
        auto *verLayout = new QFormLayout(verGroup);
        verLayout->setSpacing(12);

        versionCombo_ = new QComboBox(this);
        versionCombo_->setMaxVisibleItems(15);
        versionCombo_->setEditable(true);
        versionCombo_->setInsertPolicy(QComboBox::NoInsert);
        verLayout->addRow(X4TR("create.mc"), versionCombo_);

        manifestStatusLabel_ = new QLabel(this);
        manifestStatusLabel_->setStyleSheet(QStringLiteral("color: #565f89; font-size: 11px;"));
        verLayout->addRow(QString(), manifestStatusLabel_);

        showSnapshots_ = new QCheckBox(X4TR("create.snapshots"), this);
        connect(showSnapshots_, &QCheckBox::toggled, this, &CreateInstanceDialog::populateVersions);
        verLayout->addRow(QString(), showSnapshots_);

        loaderCombo_ = new QComboBox(this);
        loaderCombo_->addItem(QStringLiteral("Vanilla"), static_cast<int>(ModLoaderType::Vanilla));
        loaderCombo_->addItem(QStringLiteral("Fabric"), static_cast<int>(ModLoaderType::Fabric));
        loaderCombo_->addItem(QStringLiteral("Forge"), static_cast<int>(ModLoaderType::Forge));
        loaderCombo_->addItem(QStringLiteral("NeoForge"), static_cast<int>(ModLoaderType::NeoForge));
        loaderCombo_->addItem(QStringLiteral("Quilt"), static_cast<int>(ModLoaderType::Quilt));
        verLayout->addRow(X4TR("create.loader"), loaderCombo_);

        loaderVersionCombo_ = new QComboBox(this);
        loaderVersionCombo_->setEnabled(false);
        loaderVersionCombo_->setMaxVisibleItems(20);
        verLayout->addRow(X4TR("create.loader_ver"), loaderVersionCombo_);

        loaderStatusLabel_ = new QLabel(this);
        loaderStatusLabel_->setStyleSheet(QStringLiteral("color: #565f89; font-size: 11px;"));
        verLayout->addRow(QString(), loaderStatusLabel_);

        layout->addWidget(verGroup);

        layout->addStretch();

        // Summary Label
        summaryLabel_ = new QLabel(this);
        summaryLabel_->setAlignment(Qt::AlignCenter);
        summaryLabel_->setStyleSheet(
            QStringLiteral("color: #bb9af7; font-weight: bold; font-size: 12px; margin-bottom: 8px;"));
        layout->addWidget(summaryLabel_);

        if (!manifest_->isLoaded()) {
            manifestStatusLabel_->setText(X4TR("create.manifest.loading"));
        } else if (versionCombo_->count() > 0) {
            manifestStatusLabel_->setText(X4TR("create.manifest.loaded"));
        } else {
            manifestStatusLabel_->setText(X4TR("create.manifest.empty"));
        }

        connect(versionCombo_, &QComboBox::currentTextChanged, this, [this](const QString &ver) {
            if (nameEdit_->text().isEmpty() || nameEdit_->text().startsWith(QStringLiteral("Minecraft "))) {
                nameEdit_->setText(QStringLiteral("Minecraft %1").arg(ver));
            }
            fetchLoaderVersions();
            updateSummary();
        });

        connect(loaderCombo_, &QComboBox::currentIndexChanged, this, [this]() {
            fetchLoaderVersions();
            updateSummary();
        });
        connect(nameEdit_, &QLineEdit::textChanged, this, &CreateInstanceDialog::updateSummary);

        auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        buttons->button(QDialogButtonBox::Ok)->setText(X4TR("btn.create"));

        connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
            if (nameEdit_->text().trimmed().isEmpty()) {
                nameEdit_->setFocus();
                return;
            }
            accept();
        });
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

        layout->addWidget(buttons);
        updateSummary();
    }

    void CreateInstanceDialog::populateVersions() {
        versionCombo_->clear();
        bool snapshots = showSnapshots_->isChecked();

        for (const auto &ver: manifest_->versions()) {
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
            loaderStatusLabel_->setText(X4TR("create.loader.none"));
            return;
        }

        QString uid;
        switch (loader) {
            case ModLoaderType::Fabric: uid = QStringLiteral("net.fabricmc.fabric-loader");
                break;
            case ModLoaderType::Forge: uid = QStringLiteral("net.minecraftforge");
                break;
            case ModLoaderType::NeoForge: uid = QStringLiteral("net.neoforged");
                break;
            case ModLoaderType::Quilt: uid = QStringLiteral("org.quiltmc.quilt-loader");
                break;
            default: return;
        }

        loaderStatusLabel_->setText(X4TR("create.loader.loading"));
        metaApi_->getPackageIndex(uid);
    }

    void CreateInstanceDialog::updateSummary() {
        QString mc = mcVersion();
        QString loaderName = loaderCombo_->currentText();
        QString icon = selectedIconKey_;

        summaryLabel_->setText(X4TR2("create.summary", mc, loaderName + QStringLiteral(" — ") + icon));
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
        return selectedIconKey_;
    }
} // namespace x4