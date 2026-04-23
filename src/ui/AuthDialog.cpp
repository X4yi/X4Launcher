#include "AuthDialog.h"
#include "../auth/AccountManager.h"
#include "../auth/AuthController.h"
#include "../core/SettingsManager.h"
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QDesktopServices>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QMessageBox>
#include <QUuid>
#include <QPixmap>

namespace x4 {

AuthDialog::AuthDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Manage Accounts"));
    setMinimumSize(500, 400);

    avatarNetwork_ = new QNetworkAccessManager(this);

    auto* layout = new QVBoxLayout(this);

    // Premium login section
    auto* premiumGroup = new QGroupBox(QStringLiteral("Premium Account (Microsoft)"), this);
    auto* premiumLayout = new QVBoxLayout(premiumGroup);
    
    loginButton_ = new QPushButton(QStringLiteral("🔐  Login with Microsoft"), this);
    loginButton_->setMinimumHeight(36);
    premiumLayout->addWidget(loginButton_);
    layout->addWidget(premiumGroup);

    // Offline account section
    auto* offlineGroup = new QGroupBox(QStringLiteral("Offline Account"), this);
    auto* offlineLayout = new QHBoxLayout(offlineGroup);
    
    offlineNameEdit_ = new QLineEdit(this);
    offlineNameEdit_->setPlaceholderText(QStringLiteral("Enter player name..."));
    offlineNameEdit_->setMinimumHeight(32);
    addOfflineButton_ = new QPushButton(QStringLiteral("➕ Add Offline"), this);
    addOfflineButton_->setMinimumHeight(32);
    offlineLayout->addWidget(offlineNameEdit_, 1);
    offlineLayout->addWidget(addOfflineButton_);
    layout->addWidget(offlineGroup);

    // Account list
    auto* listLabel = new QLabel(QStringLiteral("Accounts:"), this);
    listLabel->setStyleSheet(QStringLiteral("font-weight: bold; font-size: 13px;"));
    layout->addWidget(listLabel);

    accountList_ = new QListWidget(this);
    accountList_->setSelectionMode(QAbstractItemView::SingleSelection);
    accountList_->setIconSize(QSize(32, 32));
    accountList_->setMinimumHeight(120);
    layout->addWidget(accountList_, 1);

    // Status
    statusLabel_ = new QLabel(this);
    statusLabel_->setWordWrap(true);
    statusLabel_->setStyleSheet(QStringLiteral("color: #7aa2f7; padding: 4px;"));
    layout->addWidget(statusLabel_);

    // Bottom buttons
    auto* buttonRow = new QHBoxLayout();
    activateButton_ = new QPushButton(QStringLiteral("✔  Set Active"), this);
    removeButton_ = new QPushButton(QStringLiteral("✖  Remove"), this);
    removeButton_->setStyleSheet(QStringLiteral("color: #f7768e;"));

    buttonRow->addWidget(activateButton_);
    buttonRow->addWidget(removeButton_);
    buttonRow->addStretch();
    layout->addLayout(buttonRow);

    // Connections
    connect(loginButton_, &QPushButton::clicked, this, &AuthDialog::startLogin);
    connect(addOfflineButton_, &QPushButton::clicked, this, &AuthDialog::addOfflineAccount);
    connect(offlineNameEdit_, &QLineEdit::returnPressed, this, &AuthDialog::addOfflineAccount);
    connect(removeButton_, &QPushButton::clicked, this, &AuthDialog::removeSelectedAccount);
    connect(activateButton_, &QPushButton::clicked, this, [this]() {
        auto* selected = accountList_->currentItem();
        if (!selected) return;
        QString id = selected->data(Qt::UserRole).toString();
        auth::AccountManager::instance().setActiveAccountId(id);
        refreshAccountList();
    });
    connect(accountList_, &QListWidget::itemSelectionChanged, this, &AuthDialog::onAccountSelected);

    connect(&auth::AccountManager::instance(), &auth::AccountManager::accountsChanged,
            this, &AuthDialog::refreshAccountList);
    connect(auth::AccountManager::instance().authController(), &auth::AuthController::loginStatusChanged,
            this, &AuthDialog::onLoginStatus);
    connect(auth::AccountManager::instance().authController(), &auth::AuthController::loginSuccess,
            this, [this](const AuthAccount&) { onLoginFinished(true, QStringLiteral("Login successful!")); });
    connect(auth::AccountManager::instance().authController(), &auth::AuthController::loginFailed,
            this, [this](const QString& err) { onLoginFinished(false, err); });

    loadSteveAvatar();
    refreshAccountList();
}

void AuthDialog::loadSteveAvatar() {
    // Fetch Steve's default skin face from Mojang's texture server
    QString steveUrl = QStringLiteral("https://mc-heads.net/avatar/MHF_Steve/32");
    QNetworkReply* reply = avatarNetwork_->get(QNetworkRequest(QUrl(steveUrl)));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            stevePixmap_.loadFromData(reply->readAll());
            stevePixmap_ = stevePixmap_.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            refreshAccountList(); // Refresh to apply icons
        }
    });
}

void AuthDialog::startLogin() {
    loginButton_->setEnabled(false);
    statusLabel_->setText(QStringLiteral("Opening browser..."));
    auth::AccountManager::instance().authController()->login();
}

void AuthDialog::addOfflineAccount() {
    QString name = offlineNameEdit_->text().trimmed();
    if (name.isEmpty()) {
        statusLabel_->setText(QStringLiteral("Please enter a player name."));
        return;
    }

    // Check if an offline account with this name already exists
    for (const auto& account : auth::AccountManager::instance().accounts()) {
        if (!account.isPremium && account.minecraftUsername == name) {
            statusLabel_->setText(QStringLiteral("An offline account with this name already exists."));
            return;
        }
    }

    // Create an offline account
    AuthAccount account;
    account.id = QStringLiteral("offline_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces).left(8));
    account.uuid = QUuid::createUuid().toString(QUuid::WithoutBraces).remove('-');
    account.alias = name;
    account.username = name;
    account.minecraftUsername = name;
    account.isPremium = false;
    account.accessToken = QStringLiteral("0"); // Offline token

    auth::AccountManager::instance().saveAccount(account);
    auth::AccountManager::instance().setActiveAccountId(account.id);

    offlineNameEdit_->clear();
    statusLabel_->setText(QStringLiteral("Offline account '%1' added.").arg(name));
    refreshAccountList();
}

void AuthDialog::refreshAccountList() {
    accountList_->clear();
    const auto accounts = auth::AccountManager::instance().accounts();
    QString activeId = auth::AccountManager::instance().activeAccountId();

    for (const auto& account : accounts) {
        QString text;
        if (account.isPremium) {
            text = QStringLiteral("⭐ %1").arg(account.minecraftUsername);
        } else {
            text = QStringLiteral("👤 %1 (offline)").arg(account.minecraftUsername);
        }
        if (account.id == activeId) {
            text += QStringLiteral("  ← active");
        }

        auto* item = new QListWidgetItem(text, accountList_);
        item->setData(Qt::UserRole, account.id);
        item->setSizeHint(QSize(0, 40));

        // Set avatar icon
        if (account.isPremium && !account.uuid.isEmpty()) {
            // Fetch premium skin face asynchronously
            QString avatarUrl = QStringLiteral("https://mc-heads.net/avatar/%1/32").arg(account.uuid);
            QNetworkReply* reply = avatarNetwork_->get(QNetworkRequest(QUrl(avatarUrl)));
            int row = accountList_->count() - 1;
            connect(reply, &QNetworkReply::finished, this, [this, reply, row]() {
                reply->deleteLater();
                if (reply->error() == QNetworkReply::NoError) {
                    QPixmap pix;
                    pix.loadFromData(reply->readAll());
                    if (!pix.isNull() && row < accountList_->count()) {
                        accountList_->item(row)->setIcon(QIcon(pix.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
                    }
                }
            });
        } else if (!stevePixmap_.isNull()) {
            item->setIcon(QIcon(stevePixmap_));
        }
    }
    updateDetails();
}

void AuthDialog::onAccountSelected() {
    updateDetails();
}

void AuthDialog::removeSelectedAccount() {
    auto* selected = accountList_->currentItem();
    if (!selected) return;
    QString id = selected->data(Qt::UserRole).toString();
    auth::AccountManager::instance().removeAccount(id);
    refreshAccountList();
}

void AuthDialog::onLoginStatus(const QString& status) {
    statusLabel_->setText(status);
}

void AuthDialog::onLoginFinished(bool success, const QString& message) {
    loginButton_->setEnabled(true);
    statusLabel_->setText(message);
    if (success) {
        refreshAccountList();
    }
}

void AuthDialog::updateDetails() {
    bool hasSelection = accountList_->currentItem() != nullptr;
    removeButton_->setEnabled(hasSelection);
    activateButton_->setEnabled(hasSelection);
}

} 
