#pragma once

#include <QDateTime>
#include <QJsonObject>
#include <QString>

namespace x4 {
    struct AuthAccount {
        QString id;
        QString alias;
        QString username;
        QString minecraftUsername;
        QString uuid;
        QString xuid;
        QString accessToken;
        QString refreshToken;
        QDateTime expiresAt;
        bool isPremium = false;

        bool isValid() const {
            return !id.isEmpty() && !accessToken.isEmpty() && !minecraftUsername.isEmpty();
        }

        bool isExpired() const {
            return expiresAt.isValid() && QDateTime::currentDateTimeUtc() >= expiresAt;
        }

        QJsonObject toJson() const {
            QJsonObject obj;
            obj[QStringLiteral("id")] = id;
            obj[QStringLiteral("alias")] = alias;
            obj[QStringLiteral("username")] = username;
            obj[QStringLiteral("minecraftUsername")] = minecraftUsername;
            obj[QStringLiteral("uuid")] = uuid;
            obj[QStringLiteral("xuid")] = xuid;
            obj[QStringLiteral("accessToken")] = accessToken;
            obj[QStringLiteral("refreshToken")] = refreshToken;
            obj[QStringLiteral("expiresAt")] = expiresAt.toString(Qt::ISODate);
            obj[QStringLiteral("isPremium")] = isPremium;
            return obj;
        }

        static AuthAccount fromJson(const QJsonObject &obj) {
            AuthAccount account;
            account.id = obj[QStringLiteral("id")].toString();
            account.alias = obj[QStringLiteral("alias")].toString();
            account.username = obj[QStringLiteral("username")].toString();
            account.minecraftUsername = obj[QStringLiteral("minecraftUsername")].toString();
            account.uuid = obj[QStringLiteral("uuid")].toString();
            account.xuid = obj[QStringLiteral("xuid")].toString();
            account.accessToken = obj[QStringLiteral("accessToken")].toString();
            account.refreshToken = obj[QStringLiteral("refreshToken")].toString();
            account.expiresAt = QDateTime::fromString(obj[QStringLiteral("expiresAt")].toString(), Qt::ISODate);
            account.isPremium = obj[QStringLiteral("isPremium")].toBool(false);
            return account;
        }
    };
}