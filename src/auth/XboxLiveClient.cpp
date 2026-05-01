#include "XboxLiveClient.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

namespace x4::auth {
    XboxLiveClient::XboxLiveClient(QNetworkAccessManager *network, QObject *parent)
        : QObject(parent), network_(network) {
    }

    void XboxLiveClient::authenticate(const QString &msAccessToken) {
        QJsonObject properties;
        properties[QStringLiteral("AuthMethod")] = QStringLiteral("RPS");
        properties[QStringLiteral("SiteName")] = QStringLiteral("user.auth.xboxlive.com");
        properties[QStringLiteral("RpsTicket")] = QStringLiteral("d=") + msAccessToken;

        QJsonObject body;
        body[QStringLiteral("Properties")] = properties;
        body[QStringLiteral("RelyingParty")] = QStringLiteral("http://auth.xboxlive.com");
        body[QStringLiteral("TokenType")] = QStringLiteral("JWT");

        QJsonDocument doc(body);

        QNetworkRequest request(QUrl(QStringLiteral("%1").arg(XBLEndpoint)));
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        request.setRawHeader("Accept", "application/json");

        QNetworkReply *reply = network_->post(request, doc.toJson(QJsonDocument::Compact));
        connect(reply, &QNetworkReply::finished, this, &XboxLiveClient::onReply);
    }

    void XboxLiveClient::onReply() {
        QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
        if (!reply) return;
        reply->deleteLater();

        QByteArray data = reply->readAll();
        qDebug() << "[Auth] XBL HTTP status:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        qDebug() << "[Auth] XBL response:" << data.left(500);

        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred(
                QStringLiteral("Xbox Live Error: %1 - %2").arg(reply->errorString(),
                                                               QString::fromUtf8(data.left(500))));
            return;
        }

        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(data, &error);

        if (error.error != QJsonParseError::NoError) {
            emit errorOccurred(QStringLiteral("Failed to parse XBL response: %1").arg(error.errorString()));
            return;
        }

        QJsonObject obj = doc.object();
        QString token = obj.value(QStringLiteral("Token")).toString();

        QJsonObject displayClaims = obj.value(QStringLiteral("DisplayClaims")).toObject();
        QJsonArray xui = displayClaims.value(QStringLiteral("xui")).toArray();

        if (token.isEmpty() || xui.isEmpty()) {
            emit errorOccurred(QStringLiteral("Invalid XBL token response format"));
            return;
        }

        QString userHash = xui.first().toObject().value(QStringLiteral("uhs")).toString();

        emit authenticated(token, userHash);
    }
} // namespace x4::auth