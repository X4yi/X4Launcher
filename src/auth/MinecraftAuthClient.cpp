#include "MinecraftAuthClient.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

namespace x4::auth {

MinecraftAuthClient::MinecraftAuthClient(QNetworkAccessManager* network, QObject* parent)
    : QObject(parent), network_(network) {
}

void MinecraftAuthClient::loginWithXbox(const QString& xstsToken, const QString& userHash) {
    QJsonObject body;
    body[QStringLiteral("identityToken")] = QStringLiteral("XBL3.0 x=%1;%2").arg(userHash, xstsToken);

    QJsonDocument doc(body);
    
    qDebug() << "[Auth] MC login identityToken prefix:" << QStringLiteral("XBL3.0 x=%1;%2...").arg(userHash, xstsToken.left(20));

    QNetworkRequest request(QUrl(QStringLiteral("%1").arg(LoginEndpoint)));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/json");

    QNetworkReply* reply = network_->post(request, doc.toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, &MinecraftAuthClient::onLoginReply);
}

void MinecraftAuthClient::onLoginReply() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    reply->deleteLater();

    QByteArray data = reply->readAll();
    int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    qDebug() << "[Auth] MC login HTTP status:" << httpStatus;
    qDebug() << "[Auth] MC login response:" << data.left(500);

    if (reply->error() != QNetworkReply::NoError) {
        emit errorOccurred(QStringLiteral("Minecraft Login Error (HTTP %1): %2").arg(httpStatus).arg(QString::fromUtf8(data.left(500))));
        return;
    }

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError) {
        emit errorOccurred(QStringLiteral("Failed to parse Minecraft login response: %1").arg(error.errorString()));
        return;
    }

    QJsonObject obj = doc.object();
    QString mcToken = obj.value(QStringLiteral("access_token")).toString();
    int expiresIn = obj.value(QStringLiteral("expires_in")).toInt();
    
    if (mcToken.isEmpty()) {
        emit errorOccurred(QStringLiteral("Minecraft access token is missing"));
        return;
    }

    fetchProfile(mcToken, expiresIn);
}

void MinecraftAuthClient::fetchProfile(const QString& mcAccessToken, int expiresIn) {
    QNetworkRequest request(QUrl(QStringLiteral("%1").arg(ProfileEndpoint)));
    request.setRawHeader("Authorization", QStringLiteral("Bearer %1").arg(mcAccessToken).toUtf8());

    QNetworkReply* reply = network_->get(request);
    
    // Pass mcToken and expiresIn via lambda or object properties
    connect(reply, &QNetworkReply::finished, this, [this, reply, mcAccessToken, expiresIn]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            // Error 404 typically means the account doesn't own Minecraft
            if (reply->error() == QNetworkReply::ContentNotFoundError) {
                emit errorOccurred(QStringLiteral("This account does not own Minecraft."));
                return;
            }
            emit errorOccurred(QStringLiteral("Minecraft Profile Error: %1").arg(reply->errorString()));
            return;
        }

        QByteArray data = reply->readAll();
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(data, &error);

        if (error.error != QJsonParseError::NoError) {
            emit errorOccurred(QStringLiteral("Failed to parse Minecraft profile response: %1").arg(error.errorString()));
            return;
        }

        QJsonObject obj = doc.object();
        
        MinecraftProfile profile;
        profile.id = obj.value(QStringLiteral("id")).toString();
        profile.name = obj.value(QStringLiteral("name")).toString();
        profile.accessToken = mcAccessToken;
        profile.expiresIn = expiresIn;
        profile.obtainedAt = QDateTime::currentDateTimeUtc();

        if (profile.id.isEmpty() || profile.name.isEmpty()) {
            emit errorOccurred(QStringLiteral("Invalid Minecraft profile format"));
            return;
        }

        emit authenticated(profile);
    });
}

} // namespace x4::auth
