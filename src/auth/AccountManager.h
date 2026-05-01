#pragma once

#include "AuthAccount.h"
#include <QSettings>
#include <QObject>
#include <QVector>

namespace x4::auth {
    class AuthController;

    class AccountManager : public QObject {
        Q_OBJECT

    public:
        static AccountManager &instance();

        QList<AuthAccount> accounts() const;

        const AuthAccount *account(const QString &id) const;

        QString activeAccountId() const;

        void setActiveAccountId(const QString &id);

        void removeAccount(const QString &id);

        bool hasPremiumAccount(const QString &id) const;

        void saveAccount(const AuthAccount &account);

        AuthController *authController() const;

        signals:
    

        void accountsChanged();

    private
        slots:
    

        void onLoginSuccess(const AuthAccount &account);

    private:
        explicit AccountManager(QObject *parent = nullptr);

        void loadAccounts();

        void saveAccounts();

        QSettings settings_;
        QList<AuthAccount> accounts_;
        QString activeAccountId_;

        AuthController *authController_;
    };
} // namespace x4::auth