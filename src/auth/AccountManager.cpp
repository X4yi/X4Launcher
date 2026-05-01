#include "AccountManager.h"
#include "AuthController.h"
#include "../core/PathManager.h"

#include <QJsonObject>

namespace x4::auth {
    AccountManager &AccountManager::instance() {
        static AccountManager mgr;
        return mgr;
    }

    AccountManager::AccountManager(QObject *parent)
        : QObject(parent)
          , settings_([] {
              auto &pm = PathManager::instance();
              if (pm.dataDir().isEmpty()) {
                  pm.init();
              }
              return pm.authSettingsFile();
          }(), QSettings::IniFormat)
          , authController_(new AuthController(this)) {
        settings_.setFallbacksEnabled(false);
        loadAccounts();

        connect(authController_, &AuthController::loginSuccess, this, &AccountManager::onLoginSuccess);
    }

    QList<AuthAccount> AccountManager::accounts() const {
        return accounts_;
    }

    const AuthAccount *AccountManager::account(const QString &id) const {
        for (const auto &account: accounts_) {
            if (account.id == id) return &account;
        }
        return nullptr;
    }

    QString AccountManager::activeAccountId() const {
        return activeAccountId_;
    }

    void AccountManager::setActiveAccountId(const QString &id) {
        activeAccountId_ = id;
        settings_.beginGroup(QStringLiteral("auth"));
        settings_.setValue(QStringLiteral("activeAccountId"), activeAccountId_);
        settings_.endGroup();
        emit accountsChanged();
    }

    void AccountManager::removeAccount(const QString &id) {
        auto it = std::remove_if(accounts_.begin(), accounts_.end(), [&](const AuthAccount &account) {
            return account.id == id;
        });
        if (it == accounts_.end()) return;

        accounts_.erase(it, accounts_.end());
        if (activeAccountId_ == id) activeAccountId_.clear();
        saveAccounts();

        authController_->logout(id);

        emit accountsChanged();
    }

    bool AccountManager::hasPremiumAccount(const QString &id) const {
        const AuthAccount *acct = account(id);
        return acct && acct->isValid() && acct->isPremium && !acct->isExpired();
    }

    AuthController *AccountManager::authController() const {
        return authController_;
    }

    void AccountManager::onLoginSuccess(const AuthAccount &account) {
        saveAccount(account);
        setActiveAccountId(account.id);
    }

    void AccountManager::loadAccounts() {
        settings_.beginGroup(QStringLiteral("auth"));
        settings_.beginGroup(QStringLiteral("accounts"));

        for (const auto &group: settings_.childGroups()) {
            settings_.beginGroup(group);
            QJsonObject obj;
            for (const auto &key: settings_.childKeys()) {
                QVariant val = settings_.value(key);
                if (key == QStringLiteral("isPremium")) {
                    obj[key] = val.toBool();
                } else {
                    obj[key] = val.toString();
                }
            }
            settings_.endGroup();
            accounts_.append(AuthAccount::fromJson(obj));
        }

        settings_.endGroup();
        activeAccountId_ = settings_.value(QStringLiteral("activeAccountId")).toString();
        settings_.endGroup();
    }

    void AccountManager::saveAccounts() {
        settings_.beginGroup(QStringLiteral("auth"));
        settings_.remove(QStringLiteral("accounts"));
        settings_.beginGroup(QStringLiteral("accounts"));
        for (const auto &account: std::as_const(accounts_)) {
            settings_.beginGroup(account.id);
            auto obj = account.toJson();
            for (const auto &key: obj.keys()) {
                settings_.setValue(key, obj.value(key).toVariant());
            }
            settings_.endGroup();
        }
        settings_.endGroup();
        settings_.setValue(QStringLiteral("activeAccountId"), activeAccountId_);
        settings_.endGroup();
    }

    void AccountManager::saveAccount(const AuthAccount &account) {
        auto existing = std::find_if(accounts_.begin(), accounts_.end(), [&](const AuthAccount &item) {
            return item.id == account.id;
        });
        if (existing != accounts_.end()) {
            *existing = account;
        } else {
            accounts_.append(account);
        }
        saveAccounts();
        emit accountsChanged();
    }
} // namespace x4::auth